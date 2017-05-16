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

struct pstreamer {
	char source_ip[MAX_IPADDR_LENGTH];
	char source_port[MAX_PORT_LENGTH];
	char id[PSID_LENGTH];  // identifier for the streamer instance
	struct pscontext * psc;
	time_t last_beat;
};

struct pstreamer_manager {
	struct ord_set * streamers;
};

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
	char fmt[] = "{\"id\":\"%s\",\"source_ip\":\"%s\",\"source_port\":\"%s\"}";
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

struct pstreamer_manager * pstreamer_manager_new()
{
	struct pstreamer_manager * psm = NULL;

	psm = malloc(sizeof(struct pstreamer_manager));
	psm->streamers = ord_set_new(1, pstreamer_cmp);

	return psm;
}

void pstreamer_manager_destroy(struct pstreamer_manager ** psm)
{
	if (psm && *psm)
	{
		ord_set_destroy(&((*psm)->streamers), 1);
		free(*psm);
		*psm = NULL;
	}
}

void pstreamer_touch(struct pstreamer *ps)
{
	ps->last_beat = time(NULL);
}

const struct pstreamer * pstreamer_manager_create_streamer(struct pstreamer_manager * psm, const char * source_ip, const char * source_port, const char * id)
{
	struct pstreamer * ps = NULL;
	const void * ptr = NULL;

	if (psm && source_ip && source_port && id)
	{
		ps = malloc(sizeof(struct pstreamer));
		strncpy(ps->source_ip, source_ip, MAX_IPADDR_LENGTH);
		strncpy(ps->source_port, source_port, MAX_PORT_LENGTH);
		strncpy(ps->id, id, PSID_LENGTH);
		ptr = ord_set_find(psm->streamers, (const void *) ps);
		if (ptr == NULL)
		{
			pstreamer_touch(ps);
			// initialize context
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
		res = ord_set_remove(psm->streamers, ps, 1);

	return res;
}
