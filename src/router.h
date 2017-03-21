/*******************************************************************
* PeerStreamer-ng is a P2P video streaming application exposing a ReST
* interface.
* Copyright (C) 2016 Luca Baldesi <luca.baldesi@unitn.it>
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

#ifndef _ROUTER_H_ 
#define _ROUTER_H_ 

#include <stdint.h>

#include <mongoose.h>


typedef void (*http_handle_t)(struct mg_connection *nc, struct http_message *hm);

/** Create a router object with initial capability <size>. 
 *  Capability is automatically increased when needed.
 *  Returns: the newly created structure.
 **/
struct router * router_create(uint32_t size);

void router_destroy(struct router ** r);

/** Add a new route handler.
 *	<method>: HTTP verb, e.g., GET, POST,...
 *	<regex>: path matching regex, e.g., /users/[a-d]+/posts/[0-9]+
 *	Returns: 0 on success
 **/
uint8_t router_add_route(struct router *r, const char * method, const char * regex, http_handle_t handler);

/** Launch the handler whose handler matches the message URL.
 *  Returns: 0 on success (handler found and launched)
 **/
uint8_t router_handle(const struct router * r, struct mg_connection *nc, struct http_message *hm);

#endif
