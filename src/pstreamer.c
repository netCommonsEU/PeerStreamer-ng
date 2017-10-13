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

struct pstreamer {
	char source_ip[MAX_IPADDR_LENGTH];
	uint16_t source_port;
	char id[PSID_LENGTH];  // identifier for the streamer instance
	struct psinstance * psc;
	time_t last_beat;
	uint16_t base_port;
	struct periodic_task * topology_task;
	struct periodic_task * offer_task;
	struct periodic_task * msg_task;
	struct periodic_task * net_helper_task;
	struct task_manager * tm;
	timeout topology_interval;
};

struct pstreamer_manager {
	struct ord_set * streamers;
	uint16_t initial_streaming_port;
	char * streamer_opts;
};

int8_t pstreamer_init(struct pstreamer * ps, const char * rtp_dst_ip, const char * opts)
/* we assume source_ip and source_port are valid strings */
{
	char config[MAX_PSINSTANCE_CONFIG_LENGTH];
	char * fmt = "port=%d,dechunkiser=rtp,base=%d,addr=%s";
	int count;

	count = snprintf(config, MAX_PSINSTANCE_CONFIG_LENGTH, fmt, ps->base_port, ps->base_port+1, rtp_dst_ip);
	if (opts && (size_t)(MAX_PSINSTANCE_CONFIG_LENGTH - count) > strlen(opts))
		snprintf(config + count, MAX_PSINSTANCE_CONFIG_LENGTH - count, ",%s", opts);
	ps->psc = psinstance_create(ps->source_ip, ps->source_port, config);

	ps->topology_task = NULL;
	ps->offer_task = NULL;
	ps->msg_task = NULL;
	ps->net_helper_task = NULL;
	ps->tm = NULL;
	ps->topology_interval = 400;

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
	if (ps->msg_task)
		task_manager_destroy_task(ps->tm, &(ps->msg_task));
	if (ps->net_helper_task)
		task_manager_destroy_task(ps->tm, &(ps->net_helper_task));
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
	char fmt[] = "{\"id\":\"%s\",\"source_ip\":\"%s\",\"source_port\":\"%d\"}";
	size_t reslength;
	char * res = NULL;

	if (ps)
	{
		reslength = strlen(fmt) - 2*3 + PSID_LENGTH + MAX_IPADDR_LENGTH + MAX_PORT_LENGTH + 1;
		res = malloc(reslength * sizeof(char));
		sprintf(res, fmt, ps->id, ps->source_ip, ps->source_port);
	}
	return res;
}

struct pstreamer_manager * pstreamer_manager_new(uint16_t starting_port)
{
	struct pstreamer_manager * psm = NULL;

	psm = malloc(sizeof(struct pstreamer_manager));
	psm->streamers = ord_set_new(1, pstreamer_cmp);
	/* VLC players assume RTP port numbers are even numbers, we allocate RTP ports starting from starting_port + 1
	 * (as the first one is the pstreamer port). So starting_port must be an odd number */
	psm->initial_streaming_port = (starting_port % 2 == 1) ? starting_port : starting_port + 1;
	psm->streamer_opts = NULL;

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
			pstreamer_deinit((struct pstreamer *)ps);

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
		ps->offer_task = task_manager_new_task(tm, pstreamer_offer_task_reinit, pstreamer_offer_task_callback, psinstance_offer_interval(ps->psc), ps->psc); 
		ps->msg_task = task_manager_new_task(tm, pstreamer_msg_handling_task_reinit, pstreamer_msg_handling_task_callback, 1000, ps->psc);
		ps->net_helper_task = task_manager_new_task(tm, NULL, pstreamer_net_helper_task_callback, 10, ps->psc);
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

const struct pstreamer * pstreamer_manager_create_streamer(struct pstreamer_manager * psm, const char * source_ip, const char * source_port, const char * id, const char * rtp_dst_ip)
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
			pstreamer_init(ps, rtp_dst_ip, psm->streamer_opts);
			ord_set_insert(psm->streamers, ps, 0);
		} else
		{
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
