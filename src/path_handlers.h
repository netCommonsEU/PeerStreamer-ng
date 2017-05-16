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
#include<tokens.h>
#include<name_lengths.h>
#include<debug.h>
#include<pschannel.h>
#include<context.h>
#include<mongoose.h>

char * mg_uri_field(struct http_message *hm, uint8_t pos)
{
	char * uri;
	char ** tokens;
	uint32_t ntok;

	uri = malloc((hm->uri.len + 1) * sizeof(char));
	strncpy(uri, hm->uri.p, hm->uri.len);
	uri[hm->uri.len] = '\0';

	tokens = tokens_create(uri, '/', &ntok);
	
	strcpy(uri, tokens[pos]);
	tokens_destroy(&tokens, ntok);

	return uri;
}

void channel_index(struct mg_connection *nc, struct http_message *hm)
{
	char * channels;
	const struct context * c;

	c = (const struct context *) nc->user_data;

	debug("GET request for channels\n");
	channels = pschannel_bucket_to_json(c->pb);
	debug("\t%s\n", channels);

	mg_printf(nc, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
	mg_printf_http_chunk(nc, "%s", channels);
	mg_send_http_chunk(nc, "", 0); /* Send empty chunk, the end of response */

	free(channels);
}

void streamer_create(struct mg_connection *nc, struct http_message *hm)
{
	const struct context * c;
	char ipaddr[MAX_IPADDR_LENGTH];
	char port[MAX_PORT_LENGTH];
	char * id, *json;
	const struct pstreamer * ps;

	c = (const struct context *) nc->user_data;
	mg_get_http_var(&hm->body, "ipaddr", ipaddr, MAX_IPADDR_LENGTH);
	mg_get_http_var(&hm->body, "port", port, MAX_PORT_LENGTH);

	id = mg_uri_field(hm, 2);

	debug("POST request for resource %s\n", id);

	ps = pstreamer_manager_create_streamer(c->psm, ipaddr, port, id); 

	if(ps)
	{
		json = pstreamer_to_json(ps);
		mg_printf(nc, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
		mg_printf_http_chunk(nc, "%s", json);
		mg_send_http_chunk(nc, "", 0); /* Send empty chunk, the end of response */
		free(json);
	} else {
		mg_printf(nc, "%s", "HTTP/1.1 409 Conflict\r\nTransfer-Encoding: chunked\r\n\r\n");
		mg_send_http_chunk(nc, "", 0); /* Send empty chunk, the end of response */
	}

	free(id);
}

uint8_t load_path_handlers(struct router *r)
{
	uint8_t res = 0;

	res |= router_add_route(r, "GET", "^/channels$", channel_index);
	res |= router_add_route(r, "POST", "^/channels/[a-zA-Z0-9]+$", streamer_create);

	return res;
}
