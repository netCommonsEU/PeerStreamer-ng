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

#include <stdint.h>

#include<router.h>
#include<pschannel.h>
#include<context.h>
#include<mongoose.h>

// mg_get_http_var(&hm->body, "n1", n1, sizeof(n1));
//
void channel_index(struct mg_connection *nc, struct http_message *hm)
{
	char * channels;
	const struct context * c;

	c = (const struct context *) nc->user_data;

	channels = pschannel_bucket_to_json(c->pb);

	mg_printf(nc, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
	mg_printf_http_chunk(nc, "%s", channels);
	mg_send_http_chunk(nc, "", 0); /* Send empty chunk, the end of response */

	free(channels);
}

uint8_t load_path_handlers(struct router *r)
{
	uint8_t res = 0;

	res |= router_add_route(r, "GET", "^/channels.json$", channel_index);

	return res;
}
