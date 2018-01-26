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

#ifndef __PERIODIC_TASK_INTFS__
#define __PERIODIC_TASK_INTFS__ 1

#include<task_manager.h>
#include <sys/select.h>

uint8_t mongoose_task_callback(struct periodic_task * pt, int ret, fd_set * readfds, fd_set * writefds, fd_set * errfds);

uint8_t mongoose_task_reinit(struct periodic_task * pt);

uint8_t pstreamer_topology_task_callback(struct periodic_task * pt, int ret, fd_set * readfds, fd_set * writefds, fd_set * errfds);

uint8_t pstreamer_offer_task_callback(struct periodic_task * pt, int ret, fd_set * readfds, fd_set * writefds, fd_set * errfds);

uint8_t pstreamer_offer_task_reinit(struct periodic_task * pt);

uint8_t pstreamer_inject_task_callback(struct periodic_task * pt, int ret, fd_set * readfds, fd_set * writefds, fd_set * errfds);

uint8_t pstreamer_msg_handling_task_callback(struct periodic_task * pt, int ret, fd_set * readfds, fd_set * writefds, fd_set * errfds);

uint8_t pstreamer_msg_handling_task_reinit(struct periodic_task * pt);

uint8_t pstreamer_topology_task_callback(struct periodic_task * pt, int ret, fd_set * readfds, fd_set * writefds, fd_set * errfds);

uint8_t pschannel_populate_task_callback(struct periodic_task * pt, int ret, fd_set * readfds, fd_set * writefds, fd_set * errfds);

uint8_t pstreamer_purge_task_callback(struct periodic_task * pt, int ret, fd_set * readfds, fd_set * writefds, fd_set * errfds);

#endif
