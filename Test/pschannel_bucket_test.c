/*
 *
 *  copyright (c) 2017 luca baldesi
 *
 */

#include<assert.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>

#include"pschannel.h"


void pschannel_bucket_destroy_test()
{
	struct pschannel_bucket * pb = NULL;

	pschannel_bucket_destroy(NULL);
	pschannel_bucket_destroy(&pb);

	pb = pschannel_bucket_new(NULL);
	pschannel_bucket_destroy(&pb);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void pschannel_bucket_insert_test()
{
	struct pschannel_bucket * pb = NULL;
	uint8_t res;

	res = pschannel_bucket_insert(NULL, NULL, NULL, NULL, NULL, NULL);
	assert(res);

	pb = pschannel_bucket_new(NULL);
	res = pschannel_bucket_insert(pb, NULL, NULL, NULL, NULL, NULL);
	assert(res);
	res = pschannel_bucket_insert(pb, NULL, "10.0.0.1", "8000", NULL, NULL);
	assert(res);

	res = pschannel_bucket_insert(pb, "channel1", "10.0.0.1", "8000", "1Mbps", "localhost/channel.sdp");
	assert(res == 0);

	pschannel_bucket_destroy(&pb);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void pschannel_bucket_iter_test()
{
	struct pschannel_bucket * pb = NULL;
	const struct pschannel * iter = NULL;

	iter = pschannel_bucket_iter(NULL, NULL);
	assert(iter == NULL);

	pb = pschannel_bucket_new(NULL);
	iter = pschannel_bucket_iter(pb, iter);
	assert(iter == NULL);

	pschannel_bucket_insert(pb, "channel1", "10.0.0.1", "8000", "1Mbps", "localhost/channel.sdp");
	iter = pschannel_bucket_iter(pb, iter);
	assert(iter);
	iter = pschannel_bucket_iter(pb, iter);
	assert(iter == NULL);

	pschannel_bucket_destroy(&pb);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void pschannel_bucket_to_json_test()
{
	struct pschannel_bucket * pb = NULL;
	char * s = NULL;

	s = pschannel_bucket_to_json(pb);
	assert(s == NULL);

	pb = pschannel_bucket_new(NULL);

	s = pschannel_bucket_to_json(pb);
	assert(strcmp(s, "[]") == 0);
	free(s);

	pschannel_bucket_insert(pb, "channel1", "10.0.0.1", "8000", "1Mbps", "localhost/channel.sdp");
	s = pschannel_bucket_to_json(pb);
	assert(strcmp(s, "[{\"name\":\"channel1\",\"ipaddr\":\"10.0.0.1\",\"port\":\"8000\",\"quality\":\"1Mbps\"}]") == 0);
	free(s);

	pschannel_bucket_insert(pb, "channel2", "10.0.0.1", "8001", "1Mbps", "localhost/channel.sdp");
	s = pschannel_bucket_to_json(pb);
	assert(strcmp(s, "[{\"name\":\"channel1\",\"ipaddr\":\"10.0.0.1\",\"port\":\"8000\",\"quality\":\"1Mbps\"},{\"name\":\"channel2\",\"ipaddr\":\"10.0.0.1\",\"port\":\"8001\",\"quality\":\"1Mbps\"}]") == 0);
	free(s);

	pschannel_bucket_destroy(&pb);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void pschannel_bucket_loadfile_test()
{
	const char * testfile = "/tmp/channel_test_file.csv";
	struct pschannel_bucket * psb = NULL;
	const struct pschannel * ch;
	int8_t res = 0;
	FILE* fd;

	res = pschannel_bucket_loadfile(psb);
	assert(res == -1);

	psb = pschannel_bucket_new(NULL);
	res = pschannel_bucket_loadfile(psb);
	assert(res == -1);
	pschannel_bucket_destroy(&psb);

	psb = pschannel_bucket_new("nonexisting");
	res = pschannel_bucket_loadfile(psb);
	assert(res == -1);
	pschannel_bucket_destroy(&psb);

	fd = fopen(testfile, "w");
	fprintf(fd, "ch1,ip1,port1,q1,addr1\n");
	fclose(fd);

	psb = pschannel_bucket_new(testfile);
	res = pschannel_bucket_loadfile(psb);
	assert(res == 0);
	ch = pschannel_bucket_find(psb, "ip1", "port1");
	assert(ch);
	pschannel_bucket_destroy(&psb);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

int main(int argv, char ** argc)
{
	pschannel_bucket_destroy_test();
	pschannel_bucket_insert_test();
	pschannel_bucket_iter_test();
	pschannel_bucket_to_json_test();
	pschannel_bucket_loadfile_test();
	return 0;
}
