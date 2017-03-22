#include<malloc.h>
#include<assert.h>

#include<int_bucket.h>

void int_bucket_init_test()
{

	struct int_bucket * ib;

	ib = int_bucket_new(0);
	int_bucket_destroy(&ib);
	assert(ib==NULL);

	ib = int_bucket_new(3);
	int_bucket_destroy(&ib);
	assert(ib==NULL);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void int_bucket_insert_test()
{
	struct int_bucket * ib;

	assert(int_bucket_insert(NULL,1,1) < 0);

	ib = int_bucket_new(0);

	assert(0 == int_bucket_insert(ib,1,1));
	assert(int_bucket_length(ib) == 1);

	assert(0 == int_bucket_insert(ib,1,1));
	assert(int_bucket_length(ib) == 1);

	assert(0 == int_bucket_insert(ib,2,1));
	assert(int_bucket_length(ib) == 2);

	assert(0 == int_bucket_insert(ib,3,4));
	assert(int_bucket_length(ib) == 3);

	assert(0 == int_bucket_insert(ib,2,1));
	assert(int_bucket_length(ib) == 3);

	int_bucket_destroy(&ib);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void int_bucket_occurr_norm_test()
{
	struct int_bucket * ib;

	assert(int_bucket_occurr_norm(NULL) == 0);

	ib = int_bucket_new(0);
	assert(0 == int_bucket_insert(ib,1,1));
	assert(int_bucket_occurr_norm(ib) == 1);

	assert(0 == int_bucket_insert(ib,1,2));
	assert(int_bucket_occurr_norm(ib) == 3);

	assert(0 == int_bucket_insert(ib,10,4));
	assert(int_bucket_occurr_norm(ib) == 5);

	assert(0 == int_bucket_insert(ib,343,7));
	assert(int_bucket_occurr_norm(ib) < 8.61);
	assert(int_bucket_occurr_norm(ib) > 8.6);

	int_bucket_destroy(&ib);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void int_bucket_sum_test()
{
	struct int_bucket * ib1,* ib2;

	int_bucket_sum(NULL,NULL);

	ib1 = int_bucket_new(0);
	int_bucket_sum(ib1,NULL);
	int_bucket_sum(NULL,ib1);

	ib2 = int_bucket_new(0);
	int_bucket_sum(ib1,ib2);
	assert(int_bucket_length(ib1) == 0);
	assert(int_bucket_occurr_norm(ib1) == 0);

	int_bucket_insert(ib2,5,3);
	int_bucket_insert(ib2,67,2);
	int_bucket_insert(ib1,67,2);
	int_bucket_sum(ib1,ib2);
	assert(int_bucket_length(ib1) == 2);
	assert(int_bucket_occurr_norm(ib1) == 5);

	int_bucket_destroy(&ib1);
	int_bucket_destroy(&ib2);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void int_bucket_iter_test()
{

	struct int_bucket * ib;
	const uint32_t * iter = NULL;

	ib = int_bucket_new(5);
	iter = int_bucket_iter(ib, iter);
	assert(iter == NULL);

	int_bucket_insert(ib,5,3);
	int_bucket_insert(ib,2,1);
	int_bucket_insert(ib,7,18);

	iter = int_bucket_iter(ib, iter);
	assert(iter);
	assert(*iter == 2);

	iter = int_bucket_iter(ib, iter);
	assert(iter);
	assert(*iter == 5);

	iter = int_bucket_iter(ib, iter);
	assert(iter);
	assert(*iter == 7);

	iter = int_bucket_iter(ib, iter);
	assert(!iter);

	int_bucket_destroy(&ib);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

int main(int argc, char ** argv)
{
	int_bucket_init_test();
	int_bucket_insert_test();
	int_bucket_occurr_norm_test();
	int_bucket_sum_test();
	int_bucket_iter_test();
	return 0;
}
