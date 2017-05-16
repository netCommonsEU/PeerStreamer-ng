/*
 * This is SSSim: the Simple & Stupid Simulator
 *
 *  Copyright (c) 2015 Luca Baldesi
 *
 *  This is free software; see gpl-3.0.txt
 */

#ifndef __TOKENS_H__
#define __TOKENS_H__ 1

#include <stdint.h>

void tokens_destroy(char *** sv, uint32_t n);

char ** tokens_create(char * s, const char delim, uint32_t * n);

int32_t tokens_check(char ** tokens, uint32_t n, char * target);

#endif
