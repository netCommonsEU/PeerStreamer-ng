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

#ifndef __JANUS_INSTANCE_H__
#define __JANUS_INSTANCE_H__

#include<stdint.h>
#include<mongoose.h>
#include<task_manager.h>
#include<streamer_creation_callback.h>

struct janus_instance;
struct streamer_creation_callback;

struct janus_instance * janus_instance_create(struct mg_mgr *mongoose_srv, struct task_manager *tm, const char *config);

void janus_instance_destroy(struct janus_instance ** ji);

/*Returns 0 on success*/
int8_t janus_instance_launch(struct janus_instance * ji);

int8_t janus_instance_create_streaming_point(struct janus_instance const * janus, uint64_t *mp_id, uint16_t audio_port, uint16_t video_port, struct streamer_creation_callback *scc);

int8_t janus_instance_destroy_streaming_point(struct janus_instance const * janus, uint64_t mp_id);

#endif
