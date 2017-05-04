/* Copyright (C) 2017 Luca Baldesi <luca.baldesi@unitn.it> */

#ifndef __PERIODIC_TASK_INTFS__
#define __PERIODIC_TASK_INTFS__ 1

#include<task_manager.h>
#include <sys/select.h>

void add_fd_to_fdset(void * handler, int sock, char set);

uint8_t mongoose_task_callback(struct periodic_task * pt, int ret, fd_set * readfds, fd_set * writefds, fd_set * errfds);

uint8_t mongoose_task_reinit(struct periodic_task * pt);

#endif
