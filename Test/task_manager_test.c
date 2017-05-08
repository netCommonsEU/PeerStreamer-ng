#include<assert.h>
#include<stdio.h>
#include<task_manager.h>
#include<stdint.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>

int counter = 0;
int reset = 0;
struct sockaddr_in consumer_addr, producer_addr;
int consumer_sock, producer_sock;

int UDP_socket(struct sockaddr_in * addr, int port)
{
	int s;
	s=socket(AF_INET, SOCK_DGRAM, 0);
	memset((char *) addr, 0, sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
	inet_aton("127.0.0.1", &(addr->sin_addr));
	bind(s, (struct sockaddr *) addr, sizeof(*addr));
	return s;
}

uint8_t producer_callback(struct periodic_task * pt, int ret, fd_set * readfds, fd_set * writefds, fd_set * errfds)
{
	char msg[] = "ciao";
	sendto(producer_sock, msg, strlen(msg), 0, (struct sockaddr *) &consumer_addr, sizeof(consumer_addr));
	return 0;
}

uint8_t dummy_callback(struct periodic_task * pt, int ret, fd_set * readfds, fd_set * writefds, fd_set * errfds)
{
	counter++;
	return 0;
}

uint8_t dummy_callback2(struct periodic_task * pt, int ret, fd_set * readfds, fd_set * writefds, fd_set * errfds)
{
	counter--;
	return 0;
}

uint8_t dummy_reinit(struct periodic_task * pt)
{
	reset++;
	return 0;
}

uint8_t consumer_reinit(struct periodic_task * pt)
{
	periodic_task_readfd_add(pt, consumer_sock);
	reset++;
	return 0;
}

int stopwatch_task_manager_poll(struct task_manager * tm, timeout to, timeout * elapsed)
{
	int res;
	struct timeval t1, t2, diff;

	gettimeofday(&t1, NULL);
	res = task_manager_poll(tm, to);
	gettimeofday(&t2, NULL);
	timersub(&t2, &t1, &diff);
	*elapsed = ((diff.tv_sec) * 1000 + diff.tv_usec/1000.0) + 0.5;
	return res;
}

void task_manager_new_test()
{
	struct task_manager * tm;

	tm = task_manager_new();
	task_manager_destroy(&tm);
	assert(tm == NULL);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void task_manager_new_task_test()
{
	struct periodic_task * res;
	struct task_manager * tm;

	tm = task_manager_new();

	res = task_manager_new_task(NULL, NULL, NULL, 0, NULL);
	assert(res == NULL);

	res = task_manager_new_task(tm, NULL, NULL, 0, NULL);
	assert(res != NULL);

	res = task_manager_new_task(tm, dummy_reinit, dummy_callback, 10, NULL);
	assert(res != NULL);

	task_manager_destroy(&tm);
	reset = 0;

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void task_manager_poll_test()
{
	struct task_manager * tm;
	int res;
	timeout elapsed;

	/****** timeout part ***********/

	tm = task_manager_new();

	task_manager_new_task(tm, dummy_reinit, dummy_callback, 10, NULL);
	assert(reset == 1);

	res = task_manager_poll(NULL, 0);
	assert(res < 0);

	res = stopwatch_task_manager_poll(tm, 0, &elapsed);

	assert(res == 0);
	assert((elapsed) < 5);

	res = stopwatch_task_manager_poll(tm, 1000, &elapsed);

	assert(res == 0);
	assert((elapsed) < 15);
	assert(counter == 1);
	assert(reset == 2);

	task_manager_new_task(tm, dummy_reinit, dummy_callback2, 5, NULL);
	assert(reset == 3);

	res = stopwatch_task_manager_poll(tm, 1000, &elapsed);

	assert(res == 0);
	assert((elapsed) < 10);
	assert(counter == 0);
	assert(reset == 4);

	task_manager_destroy(&tm);

	/****** File descriptor part ***********/

	counter = 0;
	reset = 0;

	tm = task_manager_new();
	consumer_sock = UDP_socket(&consumer_addr, 6000);
	producer_sock = UDP_socket(&producer_addr, 7000);

	task_manager_new_task(tm, dummy_reinit, producer_callback, 5, NULL);
	task_manager_new_task(tm, consumer_reinit, dummy_callback, 100, NULL);
	assert(reset == 2);

	res = stopwatch_task_manager_poll(tm, 1000, &elapsed);

	assert((elapsed) < 10);
	assert(reset == 3); // producer executed

	res = stopwatch_task_manager_poll(tm, 1000, &elapsed);

	assert((elapsed) < 5); // the consumer has been waken up by the producer
	assert(reset == 4); 
	assert(counter == 1); 

	task_manager_destroy(&tm);

	counter = 0;
	reset = 0;
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

int main(int argv, char ** argc)
{
	task_manager_new_test();
	task_manager_new_task_test();
	task_manager_poll_test();
	return 0;
}
