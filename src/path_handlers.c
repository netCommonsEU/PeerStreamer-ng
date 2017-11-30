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
#include<streamer_creation_callback.h>

void mg_connection_remote_ip(char * ip, const struct mg_connection *nc)
{
	const struct sockaddr_in *sender;
	sender = (struct sockaddr_in *)&(nc->sa);
	inet_ntop(AF_INET, &(sender->sin_addr), ip, MAX_IPADDR_LENGTH);
}

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

	info("GET request for channels\n");
	channels = pschannel_bucket_to_json(c->pb);
	debug("\t%s\n", channels);

	mg_printf(nc, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\nContent-type: application/json\r\n\r\n");
	mg_printf_http_chunk(nc, "%s", channels);
	mg_send_http_chunk(nc, "", 0); /* Send empty chunk, the end of response */

	free(channels);
}

int8_t streamer_creation_handler(struct mg_connection *nc, const struct pstreamer * ps, int8_t ret)
{
	char * json = NULL;
	int8_t res = -1;

	info("Inside creation handler\n");
	if (ps)
		json = pstreamer_to_json(ps);

	if (ret == 0 && json)
	{
		mg_printf(nc, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\nContent-type: application/json\r\n\r\n");
		mg_printf_http_chunk(nc, json);
		res = 0;
		info("Stream created and served\n");
	} else {
		mg_printf(nc, "%s", "HTTP/1.1 500 Internal server error\r\nTransfer-Encoding: chunked\r\n\r\n");
		// destroy ps?
		info("Stream cannot be correctly served\n");
		if (ret)
			debug(json);
		else
			debug("PS does not exist");
	}
	if (json)
		free(json);
	mg_send_http_chunk(nc, "", 0); /* Send empty chunk, the end of response */

	return res;
}

void streamer_create(struct mg_connection *nc, struct http_message *hm)
{
	const struct context * c;
	char ipaddr[MAX_IPADDR_LENGTH];
	char rtp_dst_ip[MAX_IPADDR_LENGTH];
	char port[MAX_PORT_LENGTH];
	char * id;
	const struct pstreamer * ps = NULL;
	const struct pschannel * ch = NULL;

	c = (const struct context *) nc->user_data;
	mg_get_http_var(&hm->body, "ipaddr", ipaddr, MAX_IPADDR_LENGTH);
	mg_get_http_var(&hm->body, "port", port, MAX_PORT_LENGTH);

	id = mg_uri_field(hm, 1);
	mg_connection_remote_ip(rtp_dst_ip, nc);

	info("POST request for resource %s from %s\n", id, rtp_dst_ip);
	ch = pschannel_bucket_find(c->pb, ipaddr, port);

	if (ch)
	{
		debug("Channel: %s\n", ch->name);
		ps = pstreamer_manager_create_streamer(c->psm, ipaddr, port, id, rtp_dst_ip, streamer_creation_callback_new(nc, streamer_creation_handler)); 
		if(ps)
		{
			pstreamer_schedule_tasks((struct pstreamer*)ps, c->tm);
			info("Streamer instance created\n");
		} else {
			info("Streamer could not be launched\n");
			mg_printf(nc, "%s", "HTTP/1.1 409 Conflict\r\nTransfer-Encoding: chunked\r\n\r\n");
			mg_send_http_chunk(nc, "", 0); /* Send empty chunk, the end of response */
		}
	} else {
		info("No channel found for socket <%s:%s>\n", ipaddr, port);
		mg_printf(nc, "%s", "HTTP/1.1 404 Not Found\r\nTransfer-Encoding: chunked\r\n\r\n");
		mg_send_http_chunk(nc, "", 0); /* Send empty chunk, the end of response */
	}

	free(id);
}

void streamer_update(struct mg_connection *nc, struct http_message *hm)
{
	char * id, * json;
	const struct pstreamer * ps;
	const struct context * c;

	c = (const struct context *) nc->user_data;
	id = mg_uri_field(hm, 1);

	ps = pstreamer_manager_get_streamer(c->psm, id);
	info("UPDATE request for resource %s\n", id);
	if (ps)
	{
		pstreamer_touch((struct pstreamer*) ps);
		info("\tInstance %s found and touched\n", id);
		json = pstreamer_to_json(ps);
		mg_printf(nc, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\nContent-type: application/json\r\n\r\n");
		mg_printf_http_chunk(nc, "%s", json);
		mg_send_http_chunk(nc, "", 0); /* Send empty chunk, the end of response */
		free(json);
	} else {
		info("\tInstance %s not found\n", id);
		mg_printf(nc, "%s", "HTTP/1.1 404 Not Found\r\nTransfer-Encoding: chunked\r\n\r\n");
		mg_send_http_chunk(nc, "", 0); /* Send empty chunk, the end of response */
	}

	free(id);
}
