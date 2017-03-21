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

#ifndef __INT_BUCKET_H__
#define __INT_BUCKET_H__ 1

#include <stdint.h>

#define INT_BUCKET_INC_SIZE 10

struct int_bucket * int_bucket_new(const uint32_t size);

void int_bucket_destroy(struct int_bucket **ib);

int int_bucket_insert(struct int_bucket * ib,const uint32_t n,const uint32_t occurr);

void int_bucket_sum(struct int_bucket *dst, const struct int_bucket *op);

double int_bucket_occurr_norm(const struct int_bucket *dst);

uint32_t int_bucket_length(const struct int_bucket *ib);

const uint32_t * int_bucket_iter(const struct int_bucket *ib, const uint32_t * iter);

#endif
