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

#include<janus_instance.h>
#include<signal.h>
#include<string.h>
#include<debug.h>
#include<unistd.h>
#include<tokens.h>
#include<grapes_config.h>
#include <sys/prctl.h>  // for process sync

#define INVALID_PID -1

#define JANUS_MSG_SESSION_CREATE "{\"transaction\": \"random\", \"janus\": \"create\"}"
#define JANUS_MSG_SESSION_KEEPALIVE "{\"transaction\": \"ciao\", \"janus\": \"keepalive\"}"
#define JANUS_MSG_STREAMING_PLUGIN_CREATE "{\"transaction\": \"ciao\", \"janus\": \"attach\", \"plugin\":\"janus.plugin.streaming\"}"
#define JANUS_MSG_VIDEOROOM_PLUGIN_CREATE "{\"transaction\": \"ciao\", \"janus\": \"attach\", \"plugin\":\"janus.plugin.videoroom\"}"


struct janus_instance {
	pid_t janus_pid;
	char* endpoint;
	char * executable;
	char * conf_param;
	char * logfile;
	struct mg_mgr *mongoose_srv;
	struct periodic_task * heartbeat;
	uint64_t management_session;
	uint64_t streaming_plugin_handle;
	uint64_t videoroom_plugin_handle;
	struct task_manager * tm;
};

uint64_t janus_instance_msg_get_id(char *msg)
{
	char ** records;
	uint32_t ntoks, i;
	uint64_t res = 0;
	
	records = tokens_create(msg, ' ', &ntoks);
	if ((i = tokens_check(records, ntoks, "\"id\":")) > 0)
	{
		if (records[i+1][strlen(records[i+1])-1] == ',')
			records[i+1][strlen(records[i+1])-1] = '\0';
		sscanf(records[i+1], "%"PRId64"", &res);
		debug("ID string: %s\t ID integer: %"PRId64"\n", records[i+1], res);
	}
	tokens_destroy(&records, ntoks);
	return res;
}

char * janus_instance_streaming_handle_path(const struct janus_instance * janus)
{
	char * res = NULL;
	if (janus && janus->management_session && janus->streaming_plugin_handle && janus->endpoint)
	{
		res = malloc(sizeof(char) * (strlen(janus->endpoint) + 43));  // each identifier comprises of 20 characters at most
		sprintf(res, "%s/%"PRId64"/%"PRId64"", janus->endpoint, janus->management_session, janus->streaming_plugin_handle);
	}
	return res;
}

char * janus_instance_videoroom_handle_path(const struct janus_instance * janus)
{
	char * res = NULL;
	if (janus && janus->management_session && janus->videoroom_plugin_handle && janus->endpoint)
	{
		res = malloc(sizeof(char) * (strlen(janus->endpoint) + 43));  // each identifier comprises of 20 characters at most
		sprintf(res, "%s/%"PRId64"/%"PRId64"", janus->endpoint, janus->management_session, janus->videoroom_plugin_handle);
	}
	return res;
}

char * janus_instance_session_path(const struct janus_instance * janus)
{
	char * res = NULL;
	if (janus && janus->management_session && janus->endpoint)
	{
		res = malloc(sizeof(char) * (strlen(janus->endpoint) + 22));  // session identifier can be at most 20 characters long
		sprintf(res, "%s/%"PRId64"", janus->endpoint, janus->management_session);
	}
	return res;
}

struct janus_instance * janus_instance_create(struct mg_mgr *mongoose_srv, struct task_manager *tm, const char *config)
{
	struct janus_instance * ji = NULL;
	struct tag* tags;

	if (mongoose_srv && tm)
	{
		tags = grapes_config_parse(config);

		ji = malloc(sizeof(struct janus_instance));
		ji->endpoint = strdup(grapes_config_value_str_default(tags, "janus_endpoint", "127.0.0.1:8088/janus"));
		ji->executable = strdup(grapes_config_value_str_default(tags, "janus_executable", "Tools/janus/bin/janus"));
		ji->conf_param = strdup(grapes_config_value_str_default(tags, "janus_param", "--configs-folder=Tools/janus_conf"));
		ji->logfile = strdup(grapes_config_value_str_default(tags, "janus_logfile", "janus.log"));
		ji->janus_pid = INVALID_PID;
		ji->management_session = 0;
		ji->streaming_plugin_handle = 0;
		ji->videoroom_plugin_handle = 0;
		ji->tm = tm;
		ji->heartbeat = NULL;
		ji->mongoose_srv = mongoose_srv;

		if(tags)
			free(tags);
	}
	return ji;
}

void janus_instance_destroy(struct janus_instance ** ji)
{
	if (ji && (*ji))
	{
		if ((*ji)->janus_pid != INVALID_PID)
			kill((*ji)->janus_pid, SIGTERM);
			
		if ((*ji)->heartbeat)
			task_manager_destroy_task((*ji)->tm, &((*ji)->heartbeat));
		free((*ji)->endpoint);
		free((*ji)->executable);
		free((*ji)->conf_param);
		free((*ji)->logfile);
		free(*ji);
		*ji = NULL;
	}
}

void janus_instance_generic_handler(struct mg_connection *nc, int ev, void *ev_data)
{
	struct http_message *hm = (struct http_message *) ev_data;

	switch (ev) {
		case MG_EV_CONNECT:
			if (*(int *) ev_data != 0)
				debug("Janus communication failure\n");
			break;
		case MG_EV_HTTP_REPLY:
			switch (hm->resp_code) {
				case 200:
				default:
					debug("Janus answers: %d\n", hm->resp_code);
			}
			nc->flags |= MG_F_SEND_AND_CLOSE;
			break;
		case MG_EV_CLOSE:
			debug("Janus server closed connection\n");
			break;
		default:
			break;
	}
}

void janus_instance_streaming_plugin_handler(struct mg_connection *nc, int ev, void *ev_data)
{
	struct janus_instance * janus;
	struct http_message *hm = (struct http_message *) ev_data;
	char *buff;

	janus = nc->user_data;
	switch (ev) {
		case MG_EV_CONNECT:
			if (*(int *) ev_data != 0)
				debug("Janus communication failure\n");
			break;
		case MG_EV_HTTP_REPLY:
			switch (hm->resp_code) {
				case 200:
					buff = malloc(sizeof(char) * (hm->body.len + 1));
					strncpy(buff, hm->body.p, hm->body.len);
					buff[hm->body.len] = '\0';  // make sure string terminates
					janus->streaming_plugin_handle = janus_instance_msg_get_id(buff);
					free(buff);
					debug("Got plugin streaming_handle!\n");
				default:
					debug("Janus answers: %d\n", hm->resp_code);
			}
			nc->flags |= MG_F_SEND_AND_CLOSE;
			break;
		case MG_EV_CLOSE:
			debug("Janus server closed connection\n");
			break;
		default:
			break;
	}
}

void janus_instance_videoroom_plugin_handler(struct mg_connection *nc, int ev, void *ev_data)
{
	struct janus_instance * janus;
	struct http_message *hm = (struct http_message *) ev_data;
	char *buff;

	janus = nc->user_data;
	switch (ev) {
		case MG_EV_CONNECT:
			if (*(int *) ev_data != 0)
				debug("Janus communication failure\n");
			break;
		case MG_EV_HTTP_REPLY:
			switch (hm->resp_code) {
				case 200:
					buff = malloc(sizeof(char) * (hm->body.len + 1));
					strncpy(buff, hm->body.p, hm->body.len);
					buff[hm->body.len] = '\0';  // make sure string terminates
					janus->videoroom_plugin_handle = janus_instance_msg_get_id(buff);
					free(buff);
					debug("Got plugin videoroom_handle!\n");
				default:
					debug("Janus answers: %d\n", hm->resp_code);
			}
			nc->flags |= MG_F_SEND_AND_CLOSE;
			break;
		case MG_EV_CLOSE:
			debug("Janus server closed connection\n");
			break;
		default:
			break;
	}
}

void janus_instance_session_handler(struct mg_connection *nc, int ev, void *ev_data)
{
	struct janus_instance * janus;
	struct mg_connection * conn;
	struct http_message *hm = (struct http_message *) ev_data;
	char *buff;

	janus = nc->user_data;
	switch (ev) {
		case MG_EV_CONNECT:
			if (*(int *) ev_data != 0)
				debug("Janus communication failure\n");
			break;
		case MG_EV_HTTP_REPLY:
			switch (hm->resp_code) {
				case 200:
					buff = malloc(sizeof(char) * (hm->body.len + 1));
					strncpy(buff, hm->body.p, hm->body.len);
					buff[hm->body.len] = '\0';  // make sure string terminates
					janus->management_session = janus_instance_msg_get_id(buff);
					free(buff);
					
					// Requesting handle for the streaming plugin
					buff = malloc(sizeof(char) * (strlen(janus->endpoint) + 22));
					sprintf(buff, "%s/%"PRId64"", janus->endpoint, janus->management_session);
					conn = mg_connect_http(janus->mongoose_srv, janus_instance_streaming_plugin_handler, buff, NULL, JANUS_MSG_STREAMING_PLUGIN_CREATE);
					free(buff);
					if (conn)
						conn->user_data = (void *) janus;

					// Requesting handle for the videoroom plugin
					buff = malloc(sizeof(char) * (strlen(janus->endpoint) + 22));
					sprintf(buff, "%s/%"PRId64"", janus->endpoint, janus->management_session);
					conn = mg_connect_http(janus->mongoose_srv, janus_instance_videoroom_plugin_handler, buff, NULL, JANUS_MSG_VIDEOROOM_PLUGIN_CREATE);
					free(buff);
					if (conn)
						conn->user_data = (void *) janus;
				default:
					debug("Janus answers: %d\n", hm->resp_code);
			}
			nc->flags |= MG_F_SEND_AND_CLOSE;
			break;
		case MG_EV_CLOSE:
			debug("Janus server closed connection\n");
			break;
		default:
			break;
	}
}

int8_t	janus_instance_create_management_handle(struct janus_instance *janus)
{
	struct mg_connection * conn;
	int8_t res = -1;

	if (janus)
	{
		conn = mg_connect_http(janus->mongoose_srv, janus_instance_session_handler, janus->endpoint, NULL, JANUS_MSG_SESSION_CREATE);
		if (conn)
		{
			conn->user_data = (void *) janus;
			res = 0;
		} 
	}
	return res;
}

uint8_t janus_instance_heartbeat(struct periodic_task * pt)
{
	struct janus_instance * janus;
	struct mg_connection * conn;
	char * uri;

	janus = (struct janus_instance *) periodic_task_get_data(pt);
	uri = janus_instance_session_path(janus);
	if (uri)
	{
		conn = mg_connect_http(janus->mongoose_srv, janus_instance_generic_handler, uri, NULL, JANUS_MSG_SESSION_KEEPALIVE);
		if (conn)
			conn->user_data = (void *) janus;
		free(uri);
	}
	return 0;
}

int8_t janus_instance_launch(struct janus_instance * ji)
{
	int8_t res = -1;
	struct stat s;
	char * argv[4];
	int fd;

	if (ji && ji->janus_pid == INVALID_PID)
	{
		info("%s - %s\n", ji->executable, ji->conf_param);
		res = stat(ji->executable, &s);
		// check exe existence
		if (res == 0 && S_ISREG(s.st_mode))
		{
			ji->janus_pid = fork();
			if (ji->janus_pid != INVALID_PID)
			{
				if (ji->janus_pid) // the parent
				{
					sleep(1); // let janus bootstrap
					res = janus_instance_create_management_handle(ji);
					if (res == 0)
						ji->heartbeat = task_manager_new_task(ji->tm, janus_instance_heartbeat, NULL, 30000, ji);
				}
				else // the child
				{
					fd = creat(ji->logfile, 'w');
					dup2(fd, 1);   // make stdout go to file
					dup2(fd, 2);   // make stderr go to file - you may choose to not do this
					close(fd);

					prctl(PR_SET_PDEATHSIG, SIGHUP); // makes kernel dispatch me a SIGHUP if parent dies

					argv[0] = ji->executable;
					argv[1] = ji->conf_param;
					argv[2] = NULL;
					res = execve(ji->executable, argv, NULL);
					info("Error on launching Janus execution\n");
				}
			} else
			{
				info("Error on forking\n");
				res = -1;
			}

			
		} else
			info("Janus executable not found\n");
	}
	return res;
}

void janus_instance_streaming_point_handler(struct mg_connection *nc, int ev, void *ev_data)
{
	uint64_t * mp_id;
	struct http_message *hm = (struct http_message *) ev_data;
	char *buff;
	void ** data;
	struct streamer_creation_callback * scc;

	data = nc->user_data;
	mp_id = data[0];
	scc = data[1];
	switch (ev) {
		case MG_EV_CONNECT:
			if (*(int *) ev_data != 0)
				debug("Janus communication failure\n");
				debug("Ora triggero!\n");
			break;
		case MG_EV_HTTP_REPLY:
			switch (hm->resp_code) {
				case 200:
					buff = malloc(sizeof(char) * (hm->body.len + 1));
					strncpy(buff, hm->body.p, hm->body.len);
					buff[hm->body.len] = '\0';  // make sure string terminates
					debug(buff);
					*mp_id = janus_instance_msg_get_id(buff);
					free(buff);
				default:
					debug("Janus answers: %d\n", hm->resp_code);
			}
			nc->flags |= MG_F_SEND_AND_CLOSE;
			break;
		case MG_EV_CLOSE:
			debug("Janus server closed connection\n");
			if (scc)
			{
				debug("Janus instance calls creation trigger\n");
				streamer_creation_callback_trigger(scc, *mp_id ? 0 : 1);
				streamer_creation_callback_destroy(&scc);
				free(data);
			}
			break;
		default:
			break;
	}
}

void janus_instance_videoroom_creation_handler(struct mg_connection *nc, int ev, void *ev_data)
{
	struct http_message *hm = (struct http_message *) ev_data;
	void ** data;
	struct streamer_creation_callback * scc;

	data = nc->user_data;
	scc = data[0];
	switch (ev) {
		case MG_EV_CONNECT:
			if (*(int *) ev_data != 0)
				debug("Janus communication failure\n");
			break;
		case MG_EV_HTTP_REPLY:
			switch (hm->resp_code) {
				case 200:
					info("Room created\n");
				default:
					debug("Janus answers: %d\n", hm->resp_code);
			}
			nc->flags |= MG_F_SEND_AND_CLOSE;
			break;
		case MG_EV_CLOSE:
			debug("Janus server closed connection\n");
			if (scc)
			{
				debug("Janus instance calls creation trigger\n");
				streamer_creation_callback_trigger(scc, 0);
				streamer_creation_callback_destroy(&scc);
				free(data);
			}
			break;
		default:
			break;
	}
}

int8_t janus_instance_create_streaming_point(struct janus_instance const * janus, uint64_t *mp_id, uint16_t audio_port, uint16_t video_port, struct streamer_creation_callback *scc)
{
	struct mg_connection * conn;
	int8_t res = -1;
	char * uri;
	char * fmt = "{\"transaction\":\"random_str\",\"janus\":\"message\",\"body\":{\"request\":\"create\",\"type\":\"rtp\",\
				  \"audio\":true,\"audioport\":%"PRId16",\"audiopt\":111,\"audiortpmap\":\"opus/48000/2\",\
				  \"video\":true,\"videoport\":%"PRId16",\"videopt\":100,\"videortpmap\":\"VP8/90000\"}}";
	char buff[280];
	void ** data;

	if (janus && mp_id && audio_port && video_port)
	{
		uri = janus_instance_streaming_handle_path(janus);
		if (uri)
		{
			sprintf(buff, fmt, audio_port, video_port);
		   debug("Conctating Janus to create a new mountpoint\n");	
			conn = mg_connect_http(janus->mongoose_srv, janus_instance_streaming_point_handler, uri, NULL, buff);
			if (conn)
			{
				data = malloc(sizeof(void *) * 2);
				data[0] = mp_id;
				data[1] = scc;
				conn->user_data = data;
				res = 0;
			} else
			   debug("Aaargh, no connection!\n");	
			free(uri);
		}
	}
	return res;
}

int8_t janus_instance_destroy_streaming_point(struct janus_instance const * janus, uint64_t mp_id)
{
	struct mg_connection * conn;
	int8_t res = -1;
	char * uri;
	char * fmt = "{\"transaction\":\"random_str\",\"janus\":\"message\",\"body\":{\"request\":\"destroy\",\"id\": %"PRId64"}}";
	char buff[120];

	if (janus && mp_id)
	{
		uri = janus_instance_streaming_handle_path(janus);
		if (uri)
		{
			sprintf(buff, fmt, mp_id);
			conn = mg_connect_http(janus->mongoose_srv, janus_instance_generic_handler, uri, NULL, buff);
			if (conn)
			{
				conn->user_data = (void *) mp_id;
				res = 0;
			} 
			free(uri);
		}
	}
	return res;
}

int8_t janus_instance_create_videoroom(struct janus_instance const * janus, const char * room_id, struct streamer_creation_callback *scc)
{
	struct mg_connection * conn;
	int8_t res = -1;
	char * uri;
	char * fmt = "{\"transaction\":\"random_str\",\"janus\":\"message\",\"body\":{\"request\":\"create\",\"room\":%s}}";

	char buff[280];
	void ** data;

	if (janus && room_id)
	{
		uri = janus_instance_videoroom_handle_path(janus);
		if (uri)
		{
			sprintf(buff, fmt, room_id);
		   debug("Conctating Janus to create a new video room\n");	
			conn = mg_connect_http(janus->mongoose_srv, janus_instance_videoroom_creation_handler, uri, NULL, buff);
			if (conn)
			{
				data = malloc(sizeof(void *));
				data[0] = scc;
				conn->user_data = data;
				res = 0;
			} else
			   debug("Aaargh, no connection!\n");	
			free(uri);
		}
	}
	return res;
}

int8_t janus_instance_destroy_videoroom(struct janus_instance const * janus, const char * room_id)
{
	struct mg_connection * conn;
	int8_t res = -1;
	char * uri;
	char * fmt = "{\"transaction\":\"random_str\",\"janus\":\"message\",\"body\":{\"request\":\"destroy\",\"room\": %s}}";
	char buff[120];

	if (janus && room_id)
	{
		uri = janus_instance_videoroom_handle_path(janus);
		if (uri)
		{
			sprintf(buff, fmt, room_id);
			conn = mg_connect_http(janus->mongoose_srv, janus_instance_generic_handler, uri, NULL, buff);
			if (conn)
			{
				conn->user_data = (void *) room_id;
				res = 0;
			} 
			free(uri);
		}
	}
	return res;
}

int8_t janus_instance_forward_rtp(struct janus_instance const * janus, const char * room_id, uint64_t participant_id, const char * rtp_dest, uint16_t audio_port, uint16_t video_port)
{
	struct mg_connection * conn;
	int8_t res = -1;
	char * uri;
	char * fmt = "{\"transaction\":\"random_str\",\"janus\":\"message\",\"body\":{\"request\":\"rtp_forward\",\"room\":%s,\"publisher_id\":%"PRId64", \"host\": %s,\"audio_port\":%"PRId16",\"video_port\":%"PRId16",\"audio_pt\":111,\"video_pt\":98}}";

	char buff[280];

	if (janus && room_id && rtp_dest && audio_port > 0 && video_port > 0)
	{
		uri = janus_instance_videoroom_handle_path(janus);
		if (uri)
		{
			sprintf(buff, fmt, room_id, participant_id, rtp_dest, audio_port, video_port);
		   debug("Conctating Janus to create a new video room\n");	
			conn = mg_connect_http(janus->mongoose_srv, janus_instance_generic_handler, uri, NULL, buff);
			if (conn)
				res = 0;
			else
			   debug("Aaargh, no connection!\n");	
			free(uri);
		}
	}
	return res;
}
