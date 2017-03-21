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

#include<task_manager.h>
#include<malloc.h>
#include<int_bucket.h>
#include<list.h>
#include<sys/time.h>

typedef int sock_t;

#define TASK_NUM_INC 10
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define INVALID_SOCKET (-1)

struct periodic_task {
	periodic_task_callback callback;
	periodic_task_reinit reinit;
	timeout to;
	timeout time_to_expire;
	struct int_bucket * writefds;
	struct int_bucket * readfds;
	struct int_bucket * errfds;
	struct list_head list_el;
};

struct task_manager {
	struct list_head list;
	struct list_head * tasks;
};

struct task_manager * task_manager_new()
{
	struct task_manager * tm;

	tm = (struct task_manager *) malloc(sizeof(struct task_manager));
	tm->tasks = &(tm->list);
	INIT_LIST_HEAD(tm->tasks);

	return tm;
}

struct periodic_task * periodic_task_new(periodic_task_reinit reinit, periodic_task_callback callback, timeout to)
{
	struct periodic_task * pt;

	pt = (struct periodic_task *) malloc(sizeof(struct periodic_task));
	pt->reinit = reinit;
	pt->callback = callback;
	pt->to = to;
	pt->time_to_expire = to;
	pt->writefds = int_bucket_new(10);
	pt->readfds = int_bucket_new(10);
	pt->errfds = int_bucket_new(10);

	if (pt->reinit)
		pt->reinit(pt);
	return pt;
}

int periodic_task_reset_timeout(struct periodic_task * pt, timeout to)
{
	if(pt)
	{
		pt->to = to;
		return 0;
	}
	else
		return -1;
}

int periodic_task_writefd_add(struct periodic_task * pt, int fd)
{
	if(pt && fd > 0)
		return int_bucket_insert(pt->writefds, fd, 1);
	else
		return -1;
}

int periodic_task_readfd_add(struct periodic_task * pt, int fd)
{
	if(pt && fd > 0)
		return int_bucket_insert(pt->readfds, fd, 1);
	else
		return -1;
}

int periodic_task_errfd_add(struct periodic_task * pt, int fd)
{
	if(pt && fd > 0)
		return int_bucket_insert(pt->errfds, fd, 1);
	else
		return -1;
}

uint8_t task_manager_add(struct task_manager *tm, struct periodic_task * pt)
{
	if (tm && pt)
	{
		list_add(&(pt->list_el), tm->tasks);
		return 0;
	}
	else
	{
		return 1;
	}
}

void periodic_task_destroy(struct periodic_task ** pt)
{
	if(pt && *pt)
	{
		int_bucket_destroy(&((*pt)->writefds));
		int_bucket_destroy(&((*pt)->readfds));
		int_bucket_destroy(&((*pt)->errfds));
		free(*pt);
		*pt = NULL;
	}
}

void task_manager_destroy(struct task_manager ** tm)
{
	struct periodic_task *pos, *n;

	if (tm && *tm)
	{
		list_for_each_entry_safe(pos, n, (*tm)->tasks, list_el)
			periodic_task_destroy(&(pos));
		free(*tm);
		*tm = NULL;
	}
}

struct periodic_task * task_manager_new_task(struct task_manager *tm, periodic_task_reinit reinit, periodic_task_callback callback, timeout to)
{
	struct periodic_task * pt;
	uint8_t res;

	pt = periodic_task_new(reinit, callback, to);
	res = task_manager_add(tm, pt);
	if (res)
	{
		periodic_task_destroy(&pt);
		pt = NULL;
	}
	return pt;
}

void task_manager_int_bucket2fd_set(const struct int_bucket *fd_int, fd_set * fds, sock_t * max_fd)
{
	const uint32_t * fd;
	fd = NULL;
	while((fd = int_bucket_iter(fd_int, fd)))
		{
			FD_SET(*fd, fds);
			if((*max_fd) == INVALID_SOCKET || *fd - (*max_fd) > 0)
				*max_fd = *fd;
		}
}

void periodic_task_trigger(struct periodic_task * pt, int event, fd_set * read_set, fd_set *write_set, fd_set *err_set)
{
	if (pt)
	{
		if(pt->callback)
			pt->callback(event, read_set, write_set, err_set);
		if(pt->reinit)
			pt->reinit(pt);
		pt->time_to_expire = pt->to;
	}
}

uint8_t periodic_task_triggerable(const struct periodic_task *pt, fd_set * read_set, fd_set *write_set, fd_set *err_set)
{
	const uint32_t * iter = NULL;
	uint8_t triggerable = 0;

	while (!triggerable && (iter = int_bucket_iter(pt->readfds, iter)))
		if (FD_ISSET(*iter, read_set))
			triggerable = 1;
	while (!triggerable && (iter = int_bucket_iter(pt->writefds, iter)))
		if (FD_ISSET(*iter, write_set))
			triggerable = 1;
	while (!triggerable && (iter = int_bucket_iter(pt->errfds, iter)))
		if (FD_ISSET(*iter, err_set))
			triggerable = 1;

	return triggerable;
}

int stopwatch_select(int maxfd, fd_set * rset, fd_set * wset, fd_set * eset, struct timeval * tv, timeout * elapsed)
{
	int res;
	struct timeval t1, t2, diff;

	gettimeofday(&t1, NULL);
	res = select(maxfd, rset, wset, eset, tv);
	gettimeofday(&t2, NULL);
	timersub(&t2, &t1, &diff);
	*elapsed = ((diff.tv_sec) * 1000 + diff.tv_usec/1000.0) + 0.5;
	return res;
}

int task_manager_poll(struct task_manager * tm, timeout to)
{
	timeout sleep_time, min_to;  // milliseconds, of course
	fd_set read_set, write_set, err_set;
	sock_t max_fd;
	struct timeval tv;
	struct periodic_task * pos, *triggered_task;
	int event;

	if (tm)
	{
		max_fd = INVALID_SOCKET;
		FD_ZERO(&read_set);
		FD_ZERO(&write_set);
		FD_ZERO(&err_set);
		min_to = to;
		triggered_task = NULL;  // it means task manager got triggered for a timeout

		list_for_each_entry(pos, tm->tasks, list_el)
		{
			task_manager_int_bucket2fd_set(pos->readfds, &read_set, &max_fd);
			task_manager_int_bucket2fd_set(pos->writefds, &write_set, &max_fd);
			task_manager_int_bucket2fd_set(pos->errfds, &err_set, &max_fd);
			if (pos->time_to_expire < min_to)
			{
				min_to = pos->time_to_expire;
				triggered_task = pos;	
			}
		}

		tv.tv_sec = min_to / 1000;
		tv.tv_usec = (min_to % 1000) * 1000;

		event = stopwatch_select((int) max_fd + 1, &read_set, &write_set, &err_set, &tv, &sleep_time);

		if(event < 0)
			perror("Error on select()");

		if(event == 0)  // timeout
		{
			list_for_each_entry(pos, tm->tasks, list_el)
				pos->time_to_expire = MAX(0, pos->time_to_expire - sleep_time);
			if (triggered_task != NULL)  // not idle loop, an actual task as to perform periodic update
				periodic_task_trigger(triggered_task, event, &read_set, &write_set, &err_set);
		}

		if (event > 0)
			list_for_each_entry(pos, tm->tasks, list_el)
			{
				pos->time_to_expire = MAX(0, pos->time_to_expire - sleep_time);
				if(periodic_task_triggerable(pos, &read_set, &write_set, &err_set))
					periodic_task_trigger(pos, event, &read_set, &write_set, &err_set);
			}

		return event;
	}
	else
		return -1;
}

void task_manager_destroy_task(struct task_manager * tm, struct periodic_task ** pt)
{
	if (tm && pt && *pt)
	{
		list_del(&((*pt)->list_el));
		periodic_task_destroy(pt);
	}

}
