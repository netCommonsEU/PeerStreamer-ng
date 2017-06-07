/*
 *
 *  copyright (c) 2017 luca baldesi
 *
 */

#include<ord_set.h>
#include<stdlib.h>
#include<string.h>

struct ord_set {
	ord_set_size size;
	ord_set_size n_elements;
	void ** elements;
	cmp_func_t cmp;
};

ord_set_size ord_set_check_pos(const struct ord_set *h, const void * n)
{
	ord_set_size a,b,i;

	i = 0;
	if(h && h->n_elements > 0)
	{
		a = 0;
		b = h->n_elements-1;
		i = (b+a)/2;
		
		while(b > a)
		{
			if (h->cmp(n, h->elements[i]) > 0)
				a = i+1;
			if (h->cmp(n, h->elements[i]) < 0)
				b = i ? i-1 : 0;
			if (h->cmp(n, h->elements[i]) == 0)
				a = b = i;
		
			i = (b+a)/2;
		}
		if (h->cmp(n, h->elements[i]) > 0)
			i++;
	}

	return i;	
}

struct ord_set * ord_set_new(ord_set_size size, cmp_func_t cmp)
{
	struct ord_set * os = NULL;
	
	if (size && cmp)
	{
		os = (struct ord_set *) malloc (sizeof(struct ord_set));
		os->n_elements = 0;
		os->size = size;
		os->elements = (void **) malloc (sizeof(void *) * os->size);
		os->cmp = cmp;
	}
	return os;
}

void ord_set_destroy(struct ord_set ** os, uint8_t free_elements)
{
	ord_set_size i;

	if(os && *os)
	{
		if (free_elements)
			for(i = 0; i < (*os)->n_elements; i++)
				free((*os)->elements[i]);
		free((*os)->elements);
		free(*os);
		*os = NULL;
	}
}

const void * ord_set_find(const struct ord_set * os, const void * el)
{
	ord_set_size i;
	void * res = NULL;


	if(os && el)
	{
		i = ord_set_check_pos(os, el);
		if(i < os->n_elements && os->cmp(os->elements[i], el) == 0)
			res = os->elements[i];
	}
	return res;
}

uint8_t ord_set_remove(struct ord_set * os, const void * el, uint8_t free_element)
{
	uint8_t res = 1;
	ord_set_size i;

	if(os && el)
	{
		i = ord_set_check_pos(os, el);
		if(i < os->n_elements && os->cmp(os->elements[i], el) == 0)
		{
			if (free_element)
			{
				free(os->elements[i]);
				os->elements[i] = NULL;
			}
			memmove(os->elements + i, os->elements + i + 1, sizeof(void *) * (os->n_elements - i - 1));
			os->n_elements--;
			res = 0;
		}
	}
	return res;
}

void * ord_set_insert(struct ord_set * os, void * el, uint8_t override)
{
	ord_set_size i;
	void * res = NULL;

	if(os && el)
	{
		i = ord_set_check_pos(os, el);
		if(i>= os->n_elements || os->cmp(os->elements[i], el) != 0)
		{
			if((os->n_elements + 1) >= os->size)
			{
				os->size += ORD_SET_INC_SIZE;
				os->elements = (void **) realloc(os->elements,sizeof(void *) * os->size);
			}
			memmove(os->elements + i+1,os->elements + i,sizeof(void *) * (os->n_elements -i));

			os->elements[i] = el;
			os->n_elements++;
		} else
			if (override)
				os->elements[i] = el;

		res = os->elements[i];
	}
	return res;
}

const void * ord_set_iter(const struct ord_set * os, const void * iter)
{
	void * ptr = NULL;
	ord_set_size i;

	if (os)
	{
		if(iter)
		{
			i = ord_set_check_pos(os, iter);
			if (i < os->n_elements-1)
				ptr = os->elements[i+1];
		}
		else 
			if (os->n_elements > 0)
				ptr = os->elements[0];
	}
	return ptr;
}

ord_set_size ord_set_length(const struct ord_set * os)
{
	if (os)
		return os->n_elements;
	return 0;
}

int8_t ord_set_dummy_cmp(const void * p1, const void * p2)
{
	if (p1 == p2)
		return 0;
	return p1 < p2 ? -1 : 1;
}

