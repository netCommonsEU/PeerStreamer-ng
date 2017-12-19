/*******************************************************************
* PeerStreamer-ng is a P2P video streaming application exposing a ReST
* interface.
* Copyright (C) 2015 Luca Baldesi <luca.baldesi@unitn.it>
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

#ifndef __PATH_HANDLERS__ 
#define __PATH_HANDLERS__ 

#include <stdint.h>

#include<router.h>
#include<pschannel.h>


char * mg_uri_field(struct http_message *hm, uint8_t pos);

void channel_index(struct mg_connection *nc, struct http_message *hm);

void streamer_create(struct mg_connection *nc, struct http_message *hm);

void streamer_update(struct mg_connection *nc, struct http_message *hm);

void video_request(struct mg_connection *nc, struct http_message *hm);

void handle_close_connection(struct mg_connection *nc);

uint8_t load_path_handlers(struct router *r)
{
	uint8_t res = 0;

	res |= router_add_route(r, "GET", "^/channels$", channel_index);
	res |= router_add_route(r, "POST", "^/channels/[a-zA-Z0-9]+$", streamer_create);
	res |= router_add_route(r, "UPDATE", "^/channels/[a-zA-Z0-9]+$", streamer_update);
	res |= router_add_route(r, "GET", "^/channels/[a-zA-Z0-9]+\\.mp4$", video_request);

	return res;
}

#endif
