/*
 *  Copyright (c) 2015 Luca Baldesi
 *
 */

#include <assert.h>
#include <stdio.h>

#include <task_manager.h>
#include <debug.h>
#include <psinstance.h>
#include <periodic_task_intfs.h>

void psinstance_topology_test()
{
	struct psinstance *ps1, *ps2 = NULL;
	int event;

	ps1 = psinstance_create("127.0.0.1", 6000, "iface=lo,port=6001");
	assert(ps1);  // psampler sent a topology packet
	// packet is lost as destination is un-reachable
	ps2 = psinstance_create("127.0.0.1", 6001, "iface=lo,port=6000");
	assert(ps2);  // psampler sent a topology packet

	psinstance_network_periodic(ps2);
	event = psinstance_handle_msg(ps1);
	assert(event == 1);

	psinstance_destroy(&ps1);
	psinstance_destroy(&ps2);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}


void p2p_topology_test()
{
	struct task_manager * tm;
	struct psinstance *ps1, *ps2 = NULL;
	int event;

	tm = task_manager_new();
	ps1 = psinstance_create("127.0.0.1", 6000, "iface=lo,port=6001");
	assert(ps1);
	// fprintf(stderr, "PS1 (psample) sent something to 127.0.0.1:6000\n");
	// fprintf(stderr, "Destination unreachable..\n");
	ps2 = psinstance_create("127.0.0.1", 6001, "iface=lo,port=6000");
	assert(ps2);
	// fprintf(stderr, "PS2 (psample) sent something to 127.0.0.1:6001\n");

	task_manager_new_task(tm, pstreamer_net_helper_task_reinit, NULL, 100, ps2);

	task_manager_new_task(tm, pstreamer_msg_handling_task_reinit, pstreamer_msg_handling_task_callback, 100, ps2);
	event = task_manager_poll(tm, 500);
	assert(event == 0);  // PS2 cannot receive the message from PS1 as it was lost
	task_manager_new_task(tm, pstreamer_msg_handling_task_reinit, pstreamer_msg_handling_task_callback, 100, ps1);
	event = task_manager_poll(tm, 500);
	assert(event == 1);  // Received the message from PS2
	// fprintf(stderr, "PS1 (topology msg handling) sent something to PS2\n");

	psinstance_destroy(&ps1);
	psinstance_destroy(&ps2);
	task_manager_destroy(&tm);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

int main(int argv, char ** argc)
{
	set_debug(1);
	psinstance_topology_test();
	p2p_topology_test();
	return 0;
}
