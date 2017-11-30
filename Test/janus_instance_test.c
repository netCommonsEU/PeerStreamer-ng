/*
 *
 *  copyright (c) 2017 luca baldesi
 *
 */

#include<assert.h>
#include<janus_instance.h>
#include<mongoose.h>
#include<task_manager.h>
#include<unistd.h>
#include<libgen.h>

char janus_conf[6969];

void janus_instance_create_test()
{
	struct janus_instance * janus;
	struct task_manager * tm = NULL;
	struct mg_mgr * srv = NULL;

	janus = janus_instance_create(srv, tm, NULL);
	assert(janus == NULL);
	janus_instance_destroy(&janus);

	tm = task_manager_new();
	srv = (struct mg_mgr*) malloc(sizeof(struct mg_mgr));
	mg_mgr_init(srv, NULL);
	janus = janus_instance_create(srv, tm, NULL);
	assert(janus != NULL);

	janus_instance_destroy(&janus);
	task_manager_destroy(&tm);
	mg_mgr_free(srv);
	free(srv);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void janus_instance_launch_test()
{
	struct janus_instance * janus = NULL;
	struct task_manager * tm = NULL;
	struct mg_mgr * srv = NULL;
	int8_t res;

	// invalid input
	res = janus_instance_launch(janus);
	assert(res);

	// wrong file
	tm = task_manager_new();
	srv = (struct mg_mgr*) malloc(sizeof(struct mg_mgr));
	mg_mgr_init(srv, NULL);
	janus = janus_instance_create(srv, tm, "janus_executable=/tmp/97897243xxx");
	assert(janus != NULL);
	res = janus_instance_launch(janus);
	assert(res);
	janus_instance_destroy(&janus);

	// common case
	janus = janus_instance_create(srv, tm, janus_conf);
	assert(janus != NULL);
	res = janus_instance_launch(janus);
	assert(res == 0);

	sleep(1);
	janus_instance_destroy(&janus);
	task_manager_destroy(&tm);
	mg_mgr_free(srv);
	free(srv);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void janus_instance_create_streaming_point_test()
{
	struct janus_instance * janus = NULL;
	struct task_manager * tm = NULL;
	struct mg_mgr * srv = NULL;
	int8_t res;

	// invalid input
	res = janus_instance_launch(janus);
	assert(res);

	// wrong file
	tm = task_manager_new();
	srv = (struct mg_mgr*) malloc(sizeof(struct mg_mgr));
	mg_mgr_init(srv, NULL);
	janus = janus_instance_create(srv, tm, janus_conf);
	res = janus_instance_launch(janus);
	assert(res == 0);
	sleep(1);

	res = janus_instance_create_streaming_point(NULL, NULL, 0, 0, NULL);
	assert(res);
	//res = janus_instance_create_streaming_point(janus, &mountpoint, 6000, 6002, NULL);
	// HARD to test without running mongoose..
	//assert(res == 0);
	//sleep(1);
	//assert(mountpoint);

	janus_instance_destroy(&janus);
	task_manager_destroy(&tm);
	mg_mgr_free(srv);
	free(srv);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

int main(int argv, char ** argc)
{
	sprintf(janus_conf, "janus_executable=%s/../Tools/janus/bin/janus", dirname(argc[0]));

	janus_instance_create_test();
	janus_instance_launch_test();
	janus_instance_create_streaming_point_test();
	return 0;
}
