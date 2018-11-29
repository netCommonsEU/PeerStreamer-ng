/*******************************************************************
* PeerStreamer-ng is a P2P video streaming application exposing a ReST
* interface.
* Copyright (C) 2017 Luca Baldesi <luca.baldesi@unitn.it>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU Affero General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Affero General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************/

#include<pstreamer.h>
#include<time.h>
#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<ord_set.h>
#include<psinstance.h>
#include<task_manager.h>
#include<periodic_task_intfs.h>
#include<debug.h>

#define MAX_PSINSTANCE_CONFIG_LENGTH 255
#define STREAMER_PEER_CONF "port=%d,dechunkiser=rtp,audio=%d,video=%d,addr=%s"
#define STREAMER_SOURCE_CONF "port=%d,chunkiser=rtp,audio=%d,video=%d,addr=%s,max_delay_ms=25,rfc3551=1,chunk_size=10000"

struct pstreamer {
	char source_ip[MAX_IPADDR_LENGTH];
	uint16_t source_port;
	char id[PSID_LENGTH];  // identifier for the streamer instance
	struct psinstance * psc;
	time_t last_beat;
	uint16_t base_port;
	struct periodic_task * topology_task;
	struct periodic_task * offer_task;
	struct periodic_task * inject_task;
	struct periodic_task * msg_task;
	struct periodic_task * net_helper_task;
	struct task_manager * tm;
	timeout topology_interval;
	uint64_t janus_streaming_id;
	char * display_name;
};

struct pstreamer_manager {
	struct ord_set * streamers;
	uint16_t initial_streaming_port;
	char * streamer_opts;
	struct janus_instance const * janus;
};

int8_t pstreamer_init(struct pstreamer * ps, const char * rtp_ip, const char * fmt, const char * opts)
/* we assume source_ip and source_port are valid strings */
{
	char config[MAX_PSINSTANCE_CONFIG_LENGTH];
	int count;

	count = snprintf(config, MAX_PSINSTANCE_CONFIG_LENGTH, fmt, ps->base_port, ps->base_port+1, ps->base_port+3, rtp_ip);
	if (opts && (size_t)(MAX_PSINSTANCE_CONFIG_LENGTH - count) > strlen(opts))
		snprintf(config + count, MAX_PSINSTANCE_CONFIG_LENGTH - count, ",%s", opts);
	ps->psc = psinstance_create(ps->source_ip, ps->source_port, config);

	ps->topology_task = NULL;
	ps->offer_task = NULL;
	ps->inject_task = NULL;
	ps->msg_task = NULL;
	ps->net_helper_task = NULL;
	ps->tm = NULL;
	ps->topology_interval = 400;
	ps->janus_streaming_id = 0;
	ps->display_name = NULL;

	if (ps->psc)
		return 0;
	else 
		return -1;

	return 0;
}

int8_t pstreamer_deinit(struct pstreamer * ps)
{
	if (ps->topology_task)
		task_manager_destroy_task(ps->tm, &(ps->topology_task));
	if (ps->offer_task)
		task_manager_destroy_task(ps->tm, &(ps->offer_task));
	if (ps->inject_task)
		task_manager_destroy_task(ps->tm, &(ps->inject_task));
	if (ps->msg_task)
		task_manager_destroy_task(ps->tm, &(ps->msg_task));
	if (ps->net_helper_task)
		task_manager_destroy_task(ps->tm, &(ps->net_helper_task));
	if (ps->display_name)
		free(ps->display_name);	
	psinstance_destroy(&(ps->psc));
	return 0;
}

int8_t pstreamer_cmp(const void * v1, const void * v2)
{
	const struct pstreamer *ps1, *ps2;

	ps1 = (struct pstreamer *) v1;
	ps2 = (struct pstreamer *) v2;

	if (ps1 && ps2)
	{
		return strcmp(ps1->id, ps2->id);
	}
	return 0;
}

char * pstreamer_to_json(const struct pstreamer * ps)
{
	char fmt[] = "{\"id\":\"%s\",\"source_ip\":\"%s\",\"source_port\":\"%d\",\"janus_streaming_id\":\"%"PRId64"\"}";
	size_t reslength;
	char * res = NULL;

	if (ps)
	{
		reslength = strlen(fmt) - 2*3 + PSID_LENGTH + MAX_IPADDR_LENGTH + MAX_PORT_LENGTH + 1;
		res = malloc(reslength * sizeof(char));
		sprintf(res, fmt, ps->id, ps->source_ip, ps->source_port, ps->janus_streaming_id);
	}
	return res;
}

struct pstreamer_manager * pstreamer_manager_new(uint16_t starting_port, const struct janus_instance *janus)
{
	struct pstreamer_manager * psm = NULL;

	psm = malloc(sizeof(struct pstreamer_manager));
	psm->streamers = ord_set_new(1, pstreamer_cmp);
	/* VLC players assume RTP port numbers are even numbers, we allocate RTP ports starting from starting_port + 1
	 * (as the first one is the pstreamer port). So starting_port must be an odd number */
	psm->initial_streaming_port = (starting_port % 2 == 1) ? starting_port : starting_port + 1;
	psm->streamer_opts = NULL;
	psm->janus = janus;

	return psm;
}

void pstreamer_manager_remove_orphans(struct pstreamer_manager * psm, time_t interval)
{
	struct ord_set * orphans;
	struct pstreamer * ps;
	const void * iter;
	time_t now;

	now = time(NULL);
	orphans = ord_set_new(10, ord_set_dummy_cmp);
	ord_set_for_each(iter, psm->streamers)
	{
		ps = (struct pstreamer *) iter;
		if (now - ps->last_beat > interval)
			ord_set_insert(orphans, (void *) iter, 0);
	}
	ord_set_for_each(iter, orphans)
	{
		ps = (struct pstreamer *) iter;
		debug("Destroying inactive pstreamer instance %s\n", ps->id);
		pstreamer_manager_destroy_streamer(psm, ps);
	}
	ord_set_destroy(&orphans, 0);
}

void pstreamer_manager_destroy(struct pstreamer_manager ** psm)
{
	const void * ps = NULL;
	
	if (psm && *psm)
	{
		ord_set_for_each(ps, (*psm)->streamers)
		{
			if ((*psm)->janus)
			{
				if (pstreamer_is_source(ps))
					janus_instance_destroy_videoroom((*psm)->janus, ((struct pstreamer*)ps)->id);
				else
					janus_instance_destroy_streaming_point((*psm)->janus, ((struct pstreamer*)ps)->janus_streaming_id);
			}
			pstreamer_deinit((struct pstreamer *)ps);
		}

		ord_set_destroy(&((*psm)->streamers), 1);
		if((*psm)->streamer_opts)
		{
			free((*psm)->streamer_opts);
			(*psm)->streamer_opts = NULL;
		}
		free(*psm);
		*psm = NULL;
	}
}

void pstreamer_touch(struct pstreamer *ps)
{
	ps->last_beat = time(NULL);
}

int8_t pstreamer_schedule_tasks(struct pstreamer *ps, struct task_manager * tm)
{
	if (ps && tm && ps->psc)
	{
		ps->tm = tm;
		ps->topology_task = task_manager_new_task(tm, NULL, pstreamer_topology_task_callback, ps->topology_interval, ps->psc);
		ps->offer_task = task_manager_new_task(tm, pstreamer_offer_task_reinit, pstreamer_offer_task_callback, psinstance_offer_interval(ps->psc)/1000, ps->psc); 
		ps->msg_task = task_manager_new_task(tm, pstreamer_msg_handling_task_reinit, pstreamer_msg_handling_task_callback, 1000, ps->psc);
		ps->net_helper_task = task_manager_new_task(tm, pstreamer_net_helper_task_reinit, NULL, 10, ps->psc);
		if (pstreamer_is_source(ps))
			ps->inject_task = task_manager_new_task(tm, NULL, pstreamer_inject_task_callback, 3, ps->psc);
	}
	return 0;
}

uint16_t assign_streaming_ports(struct pstreamer_manager *psm)
{
	const struct pstreamer * ps = NULL;
	const void * iter = NULL;
	uint16_t base_port;
	uint8_t found = 1;

	base_port = psm->initial_streaming_port;
	while (found==1)
	{
		found = 0;
		ord_set_for_each(iter, psm->streamers)
		{
			ps = (const struct pstreamer *) iter;
			if (ps->base_port == base_port)
				found = 1;
		}
		if (found)
			base_port += 6;  /* we allocate 5 port numbers per each streamer; the first is the pstreamer port, the following 4 are the RTP ports (two
					    for audio and two for video). We add the sixth one for compatibility with the VLC players which expect the RTP base
					    port numbers to be even (base_port is kept odd) */
	}
	return base_port;
}

const struct pstreamer * pstreamer_manager_get_streamer(const struct pstreamer_manager *psm, const char * id)
{
	struct pstreamer * ps = NULL;
	const void * ptr = NULL;

	ps = malloc(sizeof(struct pstreamer));
	strncpy(ps->id, id, PSID_LENGTH);
	ptr = ord_set_find(psm->streamers, (const void *) ps);

	free(ps);
	return (const struct pstreamer*) ptr;
}

const struct pstreamer * pstreamer_manager_create_streamer(struct pstreamer_manager * psm, const char * source_ip, const char * source_port, const char * id, const char * rtp_dst_ip, struct streamer_creation_callback * scc)
{
	struct pstreamer * ps = NULL;
	const void * ptr = NULL;

	if (psm && source_ip && source_port && id && rtp_dst_ip)
	{
		ps = malloc(sizeof(struct pstreamer));
		strncpy(ps->source_ip, source_ip, MAX_IPADDR_LENGTH);
		ps->source_port = atoi(source_port);
		strncpy(ps->id, id, PSID_LENGTH);
		ps->base_port = assign_streaming_ports(psm); 
		ptr = ord_set_find(psm->streamers, (const void *) ps);
		if (ptr == NULL)
		{
			pstreamer_touch(ps);
			streamer_creation_set_pstreamer_ref(scc, ps);
			if (psm->janus)
			{
				debug("calling janus upon notification of perstreamer creation\n");
				janus_instance_create_streaming_point(psm->janus, &(ps->janus_streaming_id), ps->base_port+1, ps->base_port+3, scc);
			}
			else
				if (scc)
					streamer_creation_callback_trigger(scc, 0);
			pstreamer_init(ps, rtp_dst_ip, STREAMER_PEER_CONF, psm->streamer_opts);
			ord_set_insert(psm->streamers, ps, 0);
		} else
		{
			if (scc)
				streamer_creation_callback_destroy(&scc);
			free(ps);
			ps = NULL;
		}
	}

	return ps;
}

uint8_t pstreamer_manager_destroy_streamer(struct pstreamer_manager *psm, const struct pstreamer *ps)
{
	uint8_t res = 1;

	if (psm && ps)
	{
		if(psm->janus)
			janus_instance_destroy_streaming_point(psm->janus, ps->janus_streaming_id);
		pstreamer_deinit((struct pstreamer *)ps);
		res = ord_set_remove(psm->streamers, ps, 1);
	}

	return res;
}

const char * pstreamer_id(const struct pstreamer * ps)
{
	if (ps)
		return ps->id;
	return NULL;
}

uint16_t pstreamer_base_port(const struct pstreamer * ps)
{
	if (ps)
		return ps->base_port;
	return 0;
}

int8_t pstreamer_manager_set_streamer_options(struct pstreamer_manager *psm, const char * opts)
{
	int8_t res = -1;
	
	if (psm && opts)
	{
		if(psm->streamer_opts)
		{
			free(psm->streamer_opts);
			psm->streamer_opts = NULL;
		}
		psm->streamer_opts = strdup(opts);
		res = 0;
	}

	return res;
}

const char * pstreamer_source_ipaddr(const struct pstreamer *ps)
{
	static char ip[MAX_IPADDR_LENGTH];
	const char *res = NULL;

	if (ps)
	{
		if (pstreamer_is_source(ps))
		{
			psinstance_ip_address(ps->psc, ip, MAX_IPADDR_LENGTH);
			res = ip;
		} else
			res = ps->source_ip;
	}
	return res;
}

const char * pstreamer_source_port(const struct pstreamer *ps)
{
	static char buff[6];

	if (ps)
	{
		if (pstreamer_is_source(ps))
			sprintf(buff, "%"PRId16"", psinstance_port(ps->psc));
		else
			sprintf(buff, "%"PRId16"", ps->source_port);
		return buff;
	}
	return NULL;
}

uint8_t pstreamer_is_source(const struct pstreamer * ps)
{
	return ps && (ps->source_port == 0) ? 1 : 0;
}

const struct pstreamer * pstreamer_manager_source_iter(const struct pstreamer_manager *psm, const struct pstreamer * ps)
{
	struct pstreamer * res = (struct pstreamer *) ps;

	if (psm)
	{
		do
			res = (struct pstreamer*) ord_set_iter(psm->streamers, (const void *)res);
		while (res && !pstreamer_is_source(res));
	}
	return res;
}

char * pstreamer_manager_sources_to_json(const struct pstreamer_manager *psm)
{
	char * res = NULL, * ps_json;
	uint32_t pos;
	const void * iter;
	const struct pstreamer * ps;

	if (psm)
	{
		res = malloc(sizeof(char)*3);
		res[0] = '[';
		pos = 1;

		ord_set_for_each(iter, psm->streamers)
		{
			ps = (const struct pstreamer *) iter;
			if (pstreamer_is_source(ps))
			{
				ps_json = pstreamer_to_json(ps);
				if (ps_json)
				{
					res = realloc(res, sizeof(char)*(pos+strlen(ps_json) + (pos == 1? 2 : 3)));
					if (pos > 1)
						res[pos++] = ',';
					strcpy(res+pos, ps_json);
					pos += strlen(ps_json);
					free(ps_json);
				}
			}
		}
		res[pos++] = ']';
		res[pos] = '\0';
	}
	return res;
}

const struct pstreamer * pstreamer_manager_create_source_streamer(struct pstreamer_manager * psm, const char * id, const char * rtp_src_ip, struct streamer_creation_callback * scc)
{
	struct pstreamer * ps = NULL;
	const void * ptr = NULL;

	if (psm && id && rtp_src_ip)
	{
		ps = malloc(sizeof(struct pstreamer));
		strncpy(ps->source_ip, rtp_src_ip, MAX_IPADDR_LENGTH);
		ps->source_port = 0;
		strncpy(ps->id, id, PSID_LENGTH);
		ps->base_port = assign_streaming_ports(psm); 
		ptr = ord_set_find(psm->streamers, (const void *) ps);
		if (ptr == NULL)
		{
			pstreamer_touch(ps);
			streamer_creation_set_pstreamer_ref(scc, ps);
			if (psm->janus)
			{
				debug("calling janus upon notification of perstreamer source creation\n");
				janus_instance_create_videoroom(psm->janus, ps->id, scc);
			}
			else
				if (scc)
				{
					streamer_creation_callback_trigger(scc, 0);
					streamer_creation_callback_destroy(&scc);
				}
			pstreamer_init(ps, ps->source_ip, STREAMER_SOURCE_CONF, psm->streamer_opts);
			ord_set_insert(psm->streamers, ps, 0);
		} else
		{
			if(scc)
				streamer_creation_callback_destroy(&scc);
			free(ps);
			ps = NULL;
		}
	}

	return ps;
}

void pstreamer_source_touch(const struct pstreamer_manager *psm, struct pstreamer *ps, uint64_t janus_id)
{
	if (psm && ps)
	{
		pstreamer_touch(ps);
		if (psm->janus && pstreamer_is_source(ps))
		{
			janus_instance_forward_rtp(psm->janus, ps->id, janus_id, ps->source_ip, ps->base_port+1, ps->base_port+3);
			ps->janus_streaming_id = janus_id;
		}
	}
}

void pstreamer_set_display_name(struct pstreamer *ps, const char * name)
{
	if (ps && name)
	{
		if (ps->display_name)
			free(ps->display_name);
		ps->display_name = strdup(name);
	}
}

const char * pstreamer_get_display_name(const struct pstreamer * ps)
{
	if (ps)
		return ps->display_name;
	return NULL;
}
