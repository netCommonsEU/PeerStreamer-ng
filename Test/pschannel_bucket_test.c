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
#include"pstreamer.h"


void pschannel_bucket_destroy_test()
{
	struct pschannel_bucket * pb = NULL;

	pschannel_bucket_destroy(NULL);
	pschannel_bucket_destroy(&pb);

	pb = pschannel_bucket_new(NULL, NULL);
	pschannel_bucket_destroy(&pb);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void pschannel_bucket_insert_test()
{
	struct pschannel_bucket * pb = NULL;
	uint8_t res;

	res = pschannel_bucket_insert(NULL, NULL, NULL, NULL, NULL, NULL);
	assert(res);

	pb = pschannel_bucket_new(NULL, NULL);
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

	pb = pschannel_bucket_new(NULL, NULL);
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

	pb = pschannel_bucket_new(NULL, NULL);

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

	psb = pschannel_bucket_new(NULL, NULL);
	res = pschannel_bucket_loadfile(psb);
	assert(res == -1);
	pschannel_bucket_destroy(&psb);

	psb = pschannel_bucket_new("nonexisting", NULL);
	res = pschannel_bucket_loadfile(psb);
	assert(res == -1);
	pschannel_bucket_destroy(&psb);

	fd = fopen(testfile, "w");
	fprintf(fd, "ch1,ip1,port1,q1,addr1\n");
	fclose(fd);

	psb = pschannel_bucket_new(testfile, NULL);
	res = pschannel_bucket_loadfile(psb);
	assert(res == 0);
	ch = pschannel_bucket_find(psb, "ip1", "port1");
	assert(ch);
	pschannel_bucket_destroy(&psb);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void pschannel_load_local_streams_test()
{
	struct pstreamer_manager * psm;
	struct pschannel_bucket * pb = NULL;
	const struct pschannel * iter = NULL;
	struct pstreamer * source;

	assert(pschannel_bucket_load_local_streams(pb));

	psm = pstreamer_manager_new(6000, NULL);
	pb = pschannel_bucket_new(NULL, psm);

	// empty bucket	
	assert(pschannel_bucket_load_local_streams(pb) == 0);
	iter = pschannel_bucket_iter(pb, NULL);
	assert(iter == NULL);

	// one local source, without display name
	pstreamer_manager_create_streamer(psm, "10.0.0.1", "6000", "42", "127.0.0.1", NULL);
	source = (struct pstreamer *) pstreamer_manager_create_source_streamer(psm, "room1", "127.0.0.1", NULL);
	assert(pschannel_bucket_load_local_streams(pb) == 0);
	iter = pschannel_bucket_iter(pb, NULL);
	assert(iter == NULL);

	pstreamer_set_display_name(source, "Channel1");
	assert(pschannel_bucket_load_local_streams(pb) == 0);
	iter = pschannel_bucket_iter(pb, NULL);
	assert(iter != NULL);
	assert(strcmp(((struct pschannel *)iter)->name, "Channel1") == 0);
	iter = pschannel_bucket_iter(pb, iter);
	assert(iter == NULL);

	pstreamer_manager_destroy(&psm);
	pschannel_bucket_destroy(&pb);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void pschannel_bucket_save2file_test()
{
	struct pschannel_bucket * psb = NULL;
	const struct pschannel * ch;
	int8_t res = 0;

	res = pschannel_bucket_save2file(psb);
	assert(res);

	psb = pschannel_bucket_new(NULL, NULL);
	res = pschannel_bucket_save2file(psb);
	assert(res);
	pschannel_bucket_destroy(&psb);

	psb = pschannel_bucket_new("/tmp/channel_test_file.csv", NULL);
	pschannel_bucket_insert(psb, "local_channel", "10.0.0.1", "8000", "1Mbps", "localhost/channel.sdp");
	res = pschannel_bucket_save2file(psb);
	assert(res == 0);
	pschannel_bucket_destroy(&psb);

	psb = pschannel_bucket_new("/tmp/channel_test_file_out.csv", NULL);
	res = pschannel_bucket_loadfile(psb);
	assert(res == 0);
	ch = pschannel_bucket_find(psb, "10.0.0.1", "8000");
	assert(ch);
	assert(res == 0);
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
	pschannel_load_local_streams_test();
	pschannel_bucket_save2file_test();
	return 0;
}
