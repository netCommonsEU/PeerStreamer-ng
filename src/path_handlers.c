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

#include<stdint.h>
#include<pschannel.h>
#include<router.h>
#include<tokens.h>
#include<name_lengths.h>
#include<debug.h>
#include<pschannel.h>
#include<context.h>
#include<mongoose.h>
#include<sdpfile.h>


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
	char * id, *sdpuri;
	const struct pstreamer * ps;
	const struct pschannel * ch;

	c = (const struct context *) nc->user_data;
	mg_get_http_var(&hm->body, "ipaddr", ipaddr, MAX_IPADDR_LENGTH);
	mg_get_http_var(&hm->body, "port", port, MAX_PORT_LENGTH);

	id = mg_uri_field(hm, 1);

	debug("POST request for resource %s\n", id);
	ch = pschannel_bucket_find(c->pb, ipaddr, port);

	if (ch)
	{
		debug("Channel: %s\n", ch->name);
		ps = pstreamer_manager_create_streamer(c->psm, ipaddr, port, id); 
		if(ps)
		{
			pstreamer_schedule_tasks((struct pstreamer*)ps, c->tm);
			debug("Streamer instance created\n");
			sdpuri = sdpfile_create(c, ch, ps);
			mg_printf(nc, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
			mg_printf_http_chunk(nc, "{\"id\":\"%s\",\"name\":\"%s\",\"sdpfile\":\"%s\"}", id, ch->name, sdpuri);
			mg_send_http_chunk(nc, "", 0); /* Send empty chunk, the end of response */
			free(sdpuri);
		} else {
			debug("Streamer could not be launched\n");
			mg_printf(nc, "%s", "HTTP/1.1 409 Conflict\r\nTransfer-Encoding: chunked\r\n\r\n");
			mg_send_http_chunk(nc, "", 0); /* Send empty chunk, the end of response */
		}
	} else {
		debug("No channel found for socket <%s:%s>\n", ipaddr, port);
		mg_printf(nc, "%s", "HTTP/1.1 404 Not Found\r\nTransfer-Encoding: chunked\r\n\r\n");
		mg_send_http_chunk(nc, "", 0); /* Send empty chunk, the end of response */
	}

	free(id);
}

void streamer_update(struct mg_connection *nc, struct http_message *hm)
{
	char * id;
	const struct pstreamer * ps;
	const struct context * c;

	c = (const struct context *) nc->user_data;
	id = mg_uri_field(hm, 1);

	ps = pstreamer_manager_get_streamer(c->psm, id);
	debug("UPDATE request for resource %s\n", id);
	if (ps)
	{
		pstreamer_touch((struct pstreamer*) ps);
		debug("\tInstance %s found and touched\n", id);
	} else
		debug("\tInstance %s not found\n", id);

	free(id);
}
