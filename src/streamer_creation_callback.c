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

#include<streamer_creation_callback.h>

struct streamer_creation_callback {
	streamer_creation_handler_t callback;	
	const struct pstreamer * ps;
	const struct pschannel_bucket * psb;
	struct mg_connection *nc;
	struct mg_mgr *mgr;
};

int8_t streamer_creation_set_pstreamer_ref(struct streamer_creation_callback * scc, const struct pstreamer *ps)
{
	if (scc && ps)
	{
		scc->ps = ps;
		// scc->nc->user_data = (void *)ps;
		return 0;
	}
	return -1;
}

struct streamer_creation_callback * streamer_creation_callback_new(struct mg_connection *nc, const struct pschannel_bucket * psb, streamer_creation_handler_t handler)
{
	struct streamer_creation_callback * scc = NULL;

	if (nc && handler && psb)
	{
		scc = malloc(sizeof(struct streamer_creation_callback));
		scc->ps = NULL;
		scc->callback = handler;
		scc->nc = nc;
		scc->mgr = nc->mgr;
		scc->psb = psb;
	}
	return scc;
}

int8_t streamer_creation_callback_trigger(struct streamer_creation_callback * scc, int8_t ret)
{
	int8_t res = -1;
	struct mg_connection *nc;
	if (scc)
	{
		// we now look for the correct connection as the one we have might have
		// died in the meantime
		nc = scc->mgr->active_connections;
		while (nc && nc != scc->nc && nc->next)
			nc = nc->next;
		res = scc->callback(nc == scc->nc ? nc : NULL, scc->psb, scc->ps, ret);
	}
	return res;
}

void streamer_creation_callback_destroy(struct streamer_creation_callback ** scc)
{
	if (scc && *scc)
	{
		free(*scc);
		*scc = NULL;
	}
}


