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

#ifndef __STREAMER_CREATION_CALLBACK_H__
#define __STREAMER_CREATION_CALLBACK_H__

#include<pstreamer.h>
#include<stdint.h>
#include<mongoose.h>

struct pstreamer;
struct streamer_creation_callback;

typedef int8_t (*streamer_creation_handler_t)(struct mg_connection *nc, const struct pstreamer *ps, int8_t ret);

struct streamer_creation_callback * streamer_creation_callback_new(struct mg_connection *nc, streamer_creation_handler_t handler);
 
int8_t streamer_creation_set_pstreamer_ref(struct streamer_creation_callback * scc, const struct pstreamer *ps);

int8_t streamer_creation_callback_trigger(struct streamer_creation_callback * scc, int8_t ret);

void streamer_creation_callback_destroy(struct streamer_creation_callback ** scc);

#endif
