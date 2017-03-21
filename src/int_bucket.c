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

#include <math.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>

#include "int_bucket.h"

struct int_bucket {
	uint32_t *occurrences;
	uint32_t *integers;
	uint32_t size;
	uint32_t n_elements;
};

uint32_t int_bucket_check_pos(const struct int_bucket *h, const uint32_t n)
{
	uint32_t a,b,i;

	i = 0;
	if(h && h->n_elements > 0)
	{
		a = 0;
		b = h->n_elements-1;
		i = (b+a)/2;
		
		while(b > a)
		{
			if (n > h->integers[i])
				a = i+1;
			if (n < h->integers[i])
				b = i ? i-1 : 0;
			if (n == h->integers[i])
				a = b = i;
		
			i = (b+a)/2;
		}
		if (n > (h->integers[i]))
			i++;
	}

	return i;	
}

int int_bucket_init(struct int_bucket *ib,const uint32_t size)
{
	if(ib)
	{
		ib->size = size;
		ib->occurrences = (uint32_t *) malloc(ib->size * sizeof(uint32_t));
		ib->integers = (uint32_t *) malloc(ib->size * sizeof(uint32_t));
		ib->n_elements = 0;
	}
	return ib->occurrences == NULL && ib->integers == NULL ? -1 : 0;
}
	
struct int_bucket * int_bucket_new(const uint32_t size)
{
	struct int_bucket * ib = NULL;
	ib = (struct int_bucket *) malloc(sizeof(struct int_bucket));
	if (ib)
		int_bucket_init(ib,size);
	return ib;
}

void int_bucket_destroy(struct int_bucket **ib)
{
	if(*ib)
	{
		if(*ib != NULL && (*ib)->occurrences != NULL)
			free(((*ib)->occurrences));
		if(*ib != NULL && (*ib)->integers != NULL)
			free(((*ib)->integers));
		free(*ib);
		*ib=NULL;
	}
}

uint32_t int_bucket_length(const struct int_bucket *ib)
{
	return ib->n_elements;
}

int int_bucket_insert(struct int_bucket * ib,const uint32_t n,const uint32_t occurr)
{
	uint32_t i;

	if(ib)
	{
		i = int_bucket_check_pos(ib,n);
		if(i>= ib->n_elements || ib->integers[i] != n)
		{
			if((ib->n_elements + 1) >= ib->size)
			{
				ib->size += INT_BUCKET_INC_SIZE;
				ib->occurrences = (uint32_t *) realloc(ib->occurrences,sizeof(uint32_t) * ib->size);
				ib->integers = (uint32_t *) realloc(ib->integers,sizeof(uint32_t) * ib->size);
			}
			memmove(ib->integers + i+1,ib->integers + i,sizeof(uint32_t) * (ib->n_elements -i));
			memmove(ib->occurrences + i+1,ib->occurrences + i,sizeof(uint32_t) * (ib->n_elements -i));

			ib->integers[i] = n;
			ib->occurrences[i] = occurr;
			ib->n_elements++;
		} else
			ib->occurrences[i] += occurr;
		return 0;
	}
	return -1;
}

void int_bucket_sum(struct int_bucket *dst, const struct int_bucket *op)
{
	uint32_t i;
	if (dst && op)
		for (i = 0; i<op->n_elements;i++)
			int_bucket_insert(dst,op->integers[i],op->occurrences[i]);
}

double int_bucket_occurr_norm(const struct int_bucket *ib)
{
	double sum = 0;
	uint32_t i;

	sum = 0;
	for (i = 0; ib && i<ib->n_elements; i++)
	{
//		fprintf(stderr,"Element %d with occurrence %d\n",ib->integers[i],ib->occurrences[i]);
		sum += (ib->occurrences[i])*(ib->occurrences[i]);
	}

	return sqrt(sum);
}

const uint32_t * int_bucket_iter(const struct int_bucket *ib, const uint32_t * iter)
{
	const uint32_t * res = NULL;
	uint32_t pos;

	if (ib && (ib->n_elements))
		if (iter)
		{
			pos = int_bucket_check_pos(ib, *iter);
			if (pos < (ib->n_elements-1))
				res = iter+1;
		} else
			res = ib->integers;
	return res;
}
