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

#ifndef __TASK_MANAGER__
#define __TASK_MANAGER__ 1

#include<stdint.h>
#include<sys/select.h>
#include<mongoose.h>

typedef uint16_t timeout;

struct periodic_task;

struct task_manager;

typedef uint8_t (*periodic_task_callback)(struct periodic_task * pt, int ret, fd_set * readfds, fd_set * writefds, fd_set * errfds);

typedef uint8_t (*periodic_task_reinit)(struct periodic_task * pt);

struct task_manager * task_manager_new();

void task_manager_destroy(struct task_manager **tm);

struct periodic_task * task_manager_new_task(struct task_manager *tm, periodic_task_reinit reinit, periodic_task_callback callback, timeout to, void * data);

void task_manager_destroy_task(struct task_manager * tm, struct periodic_task ** pt);

int task_manager_poll(struct task_manager * tm, timeout to);

/**** WARNING: the following do not have test cases ********/

int periodic_task_writefd_add(struct periodic_task * pt, int fd);

int periodic_task_readfd_add(struct periodic_task * pt, int fd);

int periodic_task_errfd_add(struct periodic_task * pt, int fd);

void periodic_task_flush_fdsets(struct periodic_task *pt);

int periodic_task_set_data(struct periodic_task * pt, void * data);

void * periodic_task_get_data(const struct periodic_task * pt);

timeout periodic_task_reset_timeout(struct periodic_task * pt);

int periodic_task_set_remaining_time(struct periodic_task * pt, timeout to);

timeout periodic_task_get_remaining_time(const struct periodic_task * pt);

#endif
