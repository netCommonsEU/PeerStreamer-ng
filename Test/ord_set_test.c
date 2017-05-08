/*
 *
 *  copyright (c) 2017 luca baldesi
 *
 */

#include<malloc.h>
#include<assert.h>
#include<stdint.h>

#include"ord_set.h"

int8_t int_cmp(const void *e1, const void *e2)
{
	int *n1, *n2;

	if (e1 && e2)
	{
		n1 = (int*) e1;
		n2 = (int*) e2;
//		fprintf(stderr,"[INFO] comparing %d and %d\n", *n1, *n2);
		return *n1 - *n2;
	} else
	{
		fprintf(stderr,"[ERROR] NULL values!\n");
		return 0;
	}
}

void ord_set_new_test()
{
	struct ord_set * os;

	os = ord_set_new(0, NULL);
	assert(os == NULL);

	os = ord_set_new(10, NULL);
	assert(os == NULL);

	os = ord_set_new(0, int_cmp);
	assert(os == NULL);

	os = ord_set_new(1, int_cmp);
	assert(os != NULL);

	ord_set_destroy(&os, 0);
	assert(os == NULL);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void ord_set_insert_test()
{
	struct ord_set * os;
	void * res;
	int e1=1, e2=2, e3=2;

	res = ord_set_insert(NULL, NULL, 0);
	assert(res == NULL);
	res = ord_set_insert(NULL, NULL, 1);
	assert(res == NULL);

	os = ord_set_new(1, int_cmp);
	res = ord_set_insert(os, NULL, 0);
	assert(res == NULL);

	res = ord_set_insert(os, &e1, 0);
	assert(res == &e1);

	res = ord_set_insert(os, &e2, 0);
	assert(res == &e2);

	res = ord_set_insert(os, &e3, 0);
	assert(res == &e2);

	res = ord_set_insert(os, &e3, 1);
	assert(res == &e3);

	ord_set_destroy(&os, 0);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void ord_set_destroy_test()
{
	struct ord_set * os = NULL;
	int *e1, *e2;

	ord_set_destroy(NULL, 0);
	ord_set_destroy(&os, 0);

	os = ord_set_new(1, int_cmp);
	e1 = malloc(sizeof(int));
	e2 = malloc(sizeof(int));
	*e1 = 1;
	*e2 = 2;

	ord_set_insert(os, e1, 0);
	ord_set_insert(os, e2, 0);
	ord_set_destroy(&os, 0);

	os = ord_set_new(1, int_cmp);
	ord_set_insert(os, e1, 0);
	ord_set_insert(os, e2, 0);
	ord_set_destroy(&os, 1);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void ord_set_iter_test()
{
	struct ord_set * os;
	int e1=1, e2=12, e3=32;
	const void * iter;

	os = ord_set_new(1, int_cmp);
	ord_set_insert(os, &e3, 0);
	ord_set_insert(os, &e2, 0);
	ord_set_insert(os, &e1, 0);
	iter = NULL;
	iter = ord_set_iter(os, iter);
	assert(iter == &e1);
	iter = ord_set_iter(os, iter);
	assert(iter == &e2);
	iter = ord_set_iter(os, iter);
	assert(iter == &e3);
	iter = ord_set_iter(os, iter);
	assert(iter == NULL);

	ord_set_destroy(&os, 0);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

int main(int argv, char ** argc)
{
	ord_set_new_test();
	ord_set_insert_test();
	ord_set_destroy_test();
	ord_set_iter_test();
	return 0;
}
