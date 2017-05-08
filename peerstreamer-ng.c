/*******************************************************************
* PeerStreamer-ng is a P2P video streaming application exposing a ReST
* interface.
* Copyright (C) 2017 Luca Baldesi <luca.baldesi@unitn.it>
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

#include<signal.h>
#include<string.h>
#include<router.h>
#include<path_handlers.h>
#include<debug.h>
#include<task_manager.h>
#include<context.h>
#include<pschannel.h>
#include<mg_periodic_task_intfs.h>

#include<mongoose.h>


static uint8_t running = 1;

void sig_exit(int signo)
{
	running = 0;
}

void parse_args(struct context *c, int argc, char *const* argv)
{
	int o;

	while ((o = getopt (argc, argv, "qp:")) != -1)
		switch (o) {
			case 'p':
				strncpy(c->http_port, optarg, 16);
				break;
			case 'q':
				set_debug(0);
				break;
		}
}

void ev_handler(struct mg_connection *nc, int ev, void *ev_data)
{
	struct context *c = nc->user_data;
	struct http_message *hm;

	switch (ev) {
		case MG_EV_HTTP_REQUEST:
			hm  = (struct http_message *) ev_data;
			// Try to call a path handler. If it fails serve
			// public contents
			if(router_handle(c->router, nc, hm))
				mg_serve_http(nc, hm, c->http_opts);
			break;
		default:
			break;
	}
}

void init(struct context *c, int argc, char **argv)
{
	strncpy(c->http_port, "3000", 16);
	signal(SIGINT, sig_exit);

	c->http_opts.enable_directory_listing = "no";
	c->http_opts.document_root = "Public/";
	set_debug(1);

	c->router = router_create(10);
	load_path_handlers(c->router);
	c->tm = task_manager_new();
	c->pb = pschannel_bucket_new();
	pschannel_bucket_insert(c->pb, "local_channel", "127.0.0.1", "6000", "300kbps");

	c->mongoose_srv = (struct mg_mgr*) malloc(sizeof(struct mg_mgr));
	mg_mgr_init(c->mongoose_srv, c);

	parse_args(c, argc, argv);
}

struct mg_mgr * launch_http_task(struct context *c)
{
	struct mg_connection *nc;

	nc = mg_bind(c->mongoose_srv, c->http_port, ev_handler);
	nc->user_data = c;
	if (nc == NULL) {
		fprintf(stderr, "Error starting server on port %s\n", c->http_port);
		exit(1);
	}
	mg_set_protocol_http_websocket(nc);
	task_manager_new_task(c->tm, mongoose_task_reinit, mongoose_task_callback, 1000, (void *)c->mongoose_srv->ifaces[MG_MAIN_IFACE]);

	return c->mongoose_srv;
}

void context_deinit(struct context *c)
{
	router_destroy(&(c->router));
	task_manager_destroy(&(c->tm));
	pschannel_bucket_destroy(&(c->pb));
	mg_mgr_free(c->mongoose_srv);
	free(c->mongoose_srv);
}

int main(int argc, char** argv)
{
	static struct context c;

	init(&c, argc, argv);


	debug("Starting server on port %s\n", c.http_port);
	launch_http_task(&c);
	while (running)
		task_manager_poll(c.tm, 1000);

	debug("\nExiting..\n");
	context_deinit(&c);
	return 0;
}
