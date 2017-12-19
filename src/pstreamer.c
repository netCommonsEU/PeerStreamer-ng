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

#include<pstreamer.h>
#include <libavutil/mem.h>
#include<time.h>
#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<ord_set.h>
#include<psinstance.h>
#include<task_manager.h>
#include<msg_buffer.h>
#include<ffmuxer.h>
#include<periodic_task_intfs.h>
#include<debug.h>

#define MAX_PSINSTANCE_CONFIG_LENGTH 255

#define RTP_SOCKETS_N (4)
#define RTP_BUF_SIZE  (8192)

#define HTTP_BUFFER_START_BUF_SIZE     (0x40000)
#define HTTP_BUFFER_START_BUF_TO_US    (500000)
#define HTTP_BUFFER_TH1_SIZE           (0x0)
#define HTTP_BUFFER_TH1_TO             (0)
#define HTTP_BUFFER_TH2_SIZE           (0x0)
#define HTTP_BUFFER_TH2_TO             (0)
#define HTTP_BUFFER_FLUSH_TO_US        (1000000)

#define RTP_BUFFER_START_BUF_SIZE      (0x200000)
#define RTP_BUFFER_START_BUF_TO_US     (1000000)
#define RTP_BUFFER_TH1_SIZE            (0x0)
#define RTP_BUFFER_TH1_TO              (0)
#define RTP_BUFFER_TH2_SIZE            (0x0)
#define RTP_BUFFER_TH2_TO              (0)
#define RTP_BUFFER_FLUSH_TO_US         (1000000)

#define RTP_MUXER_HTTP_UDP_SOCKET_READY_MASK         (0x0001)
#define RTP_MUXER_HTTP_TCP_SOCKET_READY_MASK         (0x0002)
#define RTP_MUXER_HTTP_RTP_BUFFER_READY_MASK         (0x0004)
#define RTP_MUXER_HTTP_HTTP_BUFFER_READY_MASK        (0x0008)
#define RTP_MUXER_HTTP_FFMUXER_READY_MASK            (0x0010)
#define RTP_MUXER_HTTP_RTP_START_BUF_COMPLETE_MASK   (0x0020)
#define RTP_MUXER_HTTP_FFMUXER_INFO_READY_MASK       (0x0040)
#define RTP_MUXER_HTTP_FFMUXER_OUTPUT_READY_MASK     (0x0080)
#define RTP_MUXER_HTTP_FFMUXER_INIT_COMPLETE_MASK    (0x0100)
#define RTP_MUXER_HTTP_FFMUXER_CLOSING_MASK          (0x0200)
#define RTP_MUXER_HTTP_FFMUXER_CLOSED_MASK           (0x0400)
#define RTP_MUXER_HTTP_HTTP_START_BUF_COMPLETE_MASK  (0x0800)
#define RTP_MUXER_HTTP_HTTP_MGCONN_READY_MASK        (0x1000)
#define RTP_MUXER_HTTP_DEBUG_MASK                    (0x80000000)

#define RTP_SOCKET_OPEN     (0x1)
#define RTP_SOCKET_BOUND    (0x2)

#define SET_RTP_SOCKET(muxer, idx, flag) \
	((muxer)->rtp_s_state[(idx)] |= (flag))
#define CLEAR_RTP_SOCKET(muxer, idx, flag) \
	((muxer)->rtp_s_state[(idx)] &= ~(flag))
#define IS_RTP_SOCKET_SET(muxer, idx, flag) \
	((muxer)->rtp_s_state[(idx)] & (flag))

#define SET_RTP_MUXER_HTTP_STATE(muxer, flag) ((muxer)->state |= (flag))
#define CLEAR_RTP_MUXER_HTTP_STATE(muxer, flag) ((muxer)->state &= ~(flag))
#define IS_RTP_MUXER_HTTP_STATE(muxer, flag) ((muxer)->state & (flag))

#define SET_RTP_MUXER_HTTP_UDP_SOCKET_READY(muxer) \
	SET_RTP_MUXER_HTTP_STATE(muxer, RTP_MUXER_HTTP_UDP_SOCKET_READY_MASK)
#define CLEAR_RTP_MUXER_HTTP_UDP_SOCKET_READY(muxer) \
	CLEAR_RTP_MUXER_HTTP_STATE(muxer, RTP_MUXER_HTTP_UDP_SOCKET_READY_MASK)
#define IS_RTP_MUXER_HTTP_UDP_SOCKET_READY(muxer) \
	IS_RTP_MUXER_HTTP_STATE(muxer, RTP_MUXER_HTTP_UDP_SOCKET_READY_MASK)

#define SET_RTP_MUXER_HTTP_TCP_SOCKET_READY(muxer) \
	SET_RTP_MUXER_HTTP_STATE(muxer, RTP_MUXER_HTTP_TCP_SOCKET_READY_MASK)
#define CLEAR_RTP_MUXER_HTTP_TCP_SOCKET_READY(muxer) \
	CLEAR_RTP_MUXER_HTTP_STATE(muxer, RTP_MUXER_HTTP_TCP_SOCKET_READY_MASK)
#define IS_RTP_MUXER_HTTP_TCP_SOCKET_READY(muxer) \
	IS_RTP_MUXER_HTTP_STATE(muxer, RTP_MUXER_HTTP_TCP_SOCKET_READY_MASK)

#define SET_RTP_MUXER_HTTP_HTTP_BUFFER_READY(muxer) \
	SET_RTP_MUXER_HTTP_STATE(muxer, \
		RTP_MUXER_HTTP_HTTP_BUFFER_READY_MASK)
#define CLEAR_RTP_MUXER_HTTP_HTTP_BUFFER_READY(muxer) \
	CLEAR_RTP_MUXER_HTTP_STATE(muxer, \
		RTP_MUXER_HTTP_HTTP_BUFFER_READY_MASK)
#define IS_RTP_MUXER_HTTP_HTTP_BUFFER_READY(muxer) \
	IS_RTP_MUXER_HTTP_STATE(muxer, \
		RTP_MUXER_HTTP_HTTP_BUFFER_READY_MASK)

#define SET_RTP_MUXER_HTTP_RTP_BUFFER_READY(muxer) \
	SET_RTP_MUXER_HTTP_STATE(muxer, \
		RTP_MUXER_HTTP_RTP_BUFFER_READY_MASK)
#define CLEAR_RTP_MUXER_HTTP_RTP_BUFFER_READY(muxer) \
	CLEAR_RTP_MUXER_HTTP_STATE(muxer, \
		RTP_MUXER_HTTP_RTP_BUFFER_READY_MASK)
#define IS_RTP_MUXER_HTTP_RTP_BUFFER_READY(muxer) \
	IS_RTP_MUXER_HTTP_STATE(muxer, \
		RTP_MUXER_HTTP_RTP_BUFFER_READY_MASK)

#define SET_RTP_MUXER_HTTP_FFMUXER_READY(muxer) \
	SET_RTP_MUXER_HTTP_STATE(muxer, \
		RTP_MUXER_HTTP_FFMUXER_READY_MASK)
#define CLEAR_RTP_MUXER_HTTP_FFMUXER_READY(muxer) \
	CLEAR_RTP_MUXER_HTTP_STATE(muxer, \
		RTP_MUXER_HTTP_FFMUXER_READY_MASK)
#define IS_RTP_MUXER_HTTP_FFMUXER_READY(muxer) \
	IS_RTP_MUXER_HTTP_STATE(muxer, \
		RTP_MUXER_HTTP_FFMUXER_READY_MASK)

#define SET_RTP_MUXER_HTTP_FFMUXER_INFO_READY(muxer) \
	SET_RTP_MUXER_HTTP_STATE(muxer, \
		RTP_MUXER_HTTP_FFMUXER_INFO_READY_MASK)
#define CLEAR_RTP_MUXER_HTTP_FFMUXER_INFO_READY(muxer) \
	CLEAR_RTP_MUXER_HTTP_STATE(muxer, \
		RTP_MUXER_HTTP_FFMUXER_INFO_READY_MASK)
#define IS_RTP_MUXER_HTTP_FFMUXER_INFO_READY(muxer) \
	IS_RTP_MUXER_HTTP_STATE(muxer, \
		RTP_MUXER_HTTP_FFMUXER_INFO_READY_MASK)

#define SET_RTP_MUXER_HTTP_FFMUXER_OUTPUT_READY(muxer) \
	SET_RTP_MUXER_HTTP_STATE(muxer, \
		RTP_MUXER_HTTP_FFMUXER_OUTPUT_READY_MASK)
#define CLEAR_RTP_MUXER_HTTP_FFMUXER_OUTPUT_READY(muxer) \
	CLEAR_RTP_MUXER_HTTP_STATE(muxer, \
		RTP_MUXER_HTTP_FFMUXER_OUTPUT_READY_MASK)
#define IS_RTP_MUXER_HTTP_FFMUXER_OUTPUT_READY(muxer) \
	IS_RTP_MUXER_HTTP_STATE(muxer, \
		RTP_MUXER_HTTP_FFMUXER_OUTPUT_READY_MASK)

#define SET_RTP_MUXER_HTTP_FFMUXER_INIT_COMPLETE(muxer) \
	SET_RTP_MUXER_HTTP_STATE(muxer, \
		RTP_MUXER_HTTP_FFMUXER_INIT_COMPLETE_MASK)
#define CLEAR_RTP_MUXER_HTTP_FFMUXER_INIT_COMPLETE(muxer) \
	CLEAR_RTP_MUXER_HTTP_STATE(muxer, \
		RTP_MUXER_HTTP_FFMUXER_INIT_COMPLETE_MASK)
#define IS_RTP_MUXER_HTTP_FFMUXER_INIT_COMPLETE(muxer) \
	IS_RTP_MUXER_HTTP_STATE(muxer, \
		RTP_MUXER_HTTP_FFMUXER_INIT_COMPLETE_MASK)

#define SET_RTP_MUXER_HTTP_FFMUXER_CLOSING(muxer) \
	SET_RTP_MUXER_HTTP_STATE(muxer, \
		RTP_MUXER_HTTP_FFMUXER_CLOSING_MASK)
#define CLEAR_RTP_MUXER_HTTP_FFMUXER_CLOSING(muxer) \
	CLEAR_RTP_MUXER_HTTP_STATE(muxer, \
		RTP_MUXER_HTTP_FFMUXER_CLOSING_MASK)
#define IS_RTP_MUXER_HTTP_FFMUXER_CLOSING(muxer) \
	IS_RTP_MUXER_HTTP_STATE(muxer, \
		RTP_MUXER_HTTP_FFMUXER_CLOSING_MASK)

#define SET_RTP_MUXER_HTTP_FFMUXER_CLOSED(muxer) \
	SET_RTP_MUXER_HTTP_STATE(muxer, \
		RTP_MUXER_HTTP_FFMUXER_CLOSED_MASK)
#define CLEAR_RTP_MUXER_HTTP_FFMUXER_CLOSED(muxer) \
	CLEAR_RTP_MUXER_HTTP_STATE(muxer, \
		RTP_MUXER_HTTP_FFMUXER_CLOSED_MASK)
#define IS_RTP_MUXER_HTTP_FFMUXER_CLOSED(muxer) \
	IS_RTP_MUXER_HTTP_STATE(muxer, \
		RTP_MUXER_HTTP_FFMUXER_CLOSED_MASK)

#define SET_RTP_MUXER_HTTP_RTP_START_BUF_COMPLETE(muxer) \
	SET_RTP_MUXER_HTTP_STATE(muxer, \
		RTP_MUXER_HTTP_RTP_START_BUF_COMPLETE_MASK)
#define CLEAR_RTP_MUXER_HTTP_RTP_START_BUF_COMPLETE(muxer) \
	CLEAR_RTP_MUXER_HTTP_STATE(muxer, \
		RTP_MUXER_HTTP_RTP_START_BUF_COMPLETE_MASK)
#define IS_RTP_MUXER_HTTP_RTP_START_BUF_COMPLETE(muxer) \
	IS_RTP_MUXER_HTTP_STATE(muxer, \
		RTP_MUXER_HTTP_RTP_START_BUF_COMPLETE_MASK)

#define SET_RTP_MUXER_HTTP_HTTP_START_BUF_COMPLETE(muxer) \
	SET_RTP_MUXER_HTTP_STATE(muxer, \
		RTP_MUXER_HTTP_HTTP_START_BUF_COMPLETE_MASK)
#define CLEAR_RTP_MUXER_HTTP_HTTP_START_BUF_COMPLETE(muxer) \
	CLEAR_RTP_MUXER_HTTP_STATE(muxer, \
		RTP_MUXER_HTTP_HTTP_START_BUF_COMPLETE_MASK)
#define IS_RTP_MUXER_HTTP_HTTP_START_BUF_COMPLETE(muxer) \
	IS_RTP_MUXER_HTTP_STATE(muxer, \
		RTP_MUXER_HTTP_HTTP_START_BUF_COMPLETE_MASK)

#define SET_RTP_MUXER_HTTP_HTTP_MGCONN_READY(muxer) \
	SET_RTP_MUXER_HTTP_STATE(muxer, \
		RTP_MUXER_HTTP_HTTP_MGCONN_READY_MASK)
#define CLEAR_RTP_MUXER_HTTP_HTTP_MGCONN_READY(muxer) \
	CLEAR_RTP_MUXER_HTTP_STATE(muxer, \
		RTP_MUXER_HTTP_HTTP_MGCONN_READY_MASK)
#define IS_RTP_MUXER_HTTP_HTTP_MGCONN_READY(muxer) \
	IS_RTP_MUXER_HTTP_STATE(muxer, \
		RTP_MUXER_HTTP_HTTP_MGCONN_READY_MASK)

#define SET_RTP_MUXER_DEBUG(muxer) \
	SET_RTP_MUXER_HTTP_STATE(muxer, RTP_MUXER_HTTP_DEBUG_MASK)
#define CLEAR_RTP_MUXER_DEBUG(muxer) \
	CLEAR_RTP_MUXER_HTTP_STATE(muxer, RTP_MUXER_HTTP_DEBUG_MASK)
#define IS_RTP_MUXER_HTTP_DEBUG(muxer) \
	IS_RTP_MUXER_HTTP_STATE(muxer, RTP_MUXER_HTTP_DEBUG_MASK)

struct rtp_muxer_http {
	struct msg_buffer *rtpb; /* RTP msg_buffer */
	struct msg_buffer *httpb; /* HTTP msg_buffer */

	struct ffmuxer_context *ffcontext; /* ffmuxer_context */

	uint32_t state; /* See *FFMUXER_STATE* macros */

	/* UDP sockets for RTP */
	int rtp_s[RTP_SOCKETS_N];
	struct sockaddr_in rtp_sai_local[RTP_SOCKETS_N];
	uint8_t rtp_s_state[RTP_SOCKETS_N];

	struct mg_connection *nc; /* Mongoose video connection */
	char *http_root;

	double rtp_buffer_next_perc_display;
	double http_buffer_next_perc_display;
};

struct pstreamer {
	char source_ip[MAX_IPADDR_LENGTH];
	uint16_t source_port;
	char id[PSID_LENGTH];  // identifier for the streamer instance
	struct psinstance * psc;
	time_t last_beat;
	uint16_t base_port;
	struct periodic_task * topology_task;
	struct periodic_task * offer_task;
	struct periodic_task * msg_task;
	struct periodic_task * muxer_task;
	struct task_manager * tm;
	timeout topology_interval;

	struct rtp_muxer_http *http_muxer;
};

struct pstreamer_manager {
	struct ord_set * streamers;
	uint16_t initial_streaming_port;
	char * streamer_opts;
};

static int pstreamer_ffmuxer_read_packet(void *opaque,
					 uint8_t *buf, int buf_size)
{
	struct pstreamer *ps = (struct pstreamer *) opaque;
	struct rtp_muxer_http *muxer = ps->http_muxer;
	char *buffer;
	struct timeval tv;
	int buf_len;

	buf_len = msg_buffer_pop(muxer->rtpb, &buffer);

	if (buf_len > buf_size) {
		fprintf(stderr, "ERROR (%s): "
			"pstreamer_ffmuxer_read_packet() buf is too small\n",
			ps->id);
	}

	if (buf_len == 0) {
		if (msg_buffer_get_status(muxer->rtpb, &tv) ==
		    MSG_BUFFER_DATA_FLUSHED) {
			return AVERROR_EOF;
		} else {
			return 0;
		}
	}

	if (buffer) {
		memcpy(buf, buffer, buf_len);
		free(buffer);
	}

	return buf_len;
}

static int pstreamer_ffmuxer_write_packet(void *opaque,
					  uint8_t *buf, int buf_size)
{
	struct pstreamer *ps = (struct pstreamer *) opaque;

	if (IS_RTP_MUXER_HTTP_DEBUG(ps->http_muxer)) {
		fprintf(stdout, "pstreamer_ffmuxer_write_packet(): %p %p %d\n",
			opaque, buf, buf_size);
	}

	return 0;
}
static int pstreamer_ffmuxer_udp_sockets_deinit(struct pstreamer *ps)
{
	struct rtp_muxer_http *muxer = NULL;
	int ret = 0;
	int i;

	if (!ps) {
		ret = -1;
		goto err;
	}

	muxer = ps->http_muxer;

	if (!muxer) {
		ret = -1;
		goto err;
	}

	for (i = 0; i < RTP_SOCKETS_N; i++) {
		if (IS_RTP_SOCKET_SET(muxer, i, RTP_SOCKET_OPEN)) {
			close(muxer->rtp_s[i]);
		}
	}

	return ret;

err:
	return ret;
}

static int pstreamer_ffmuxer_udp_sockets_init(struct pstreamer *ps)
{
	struct rtp_muxer_http *muxer = NULL;
	int ret = 0;
	int i;

	if (!ps) {
		ret = -1;
		goto err;
	}

	muxer = ps->http_muxer;

	if (!muxer) {
		ret = -1;
		goto err;
	}

	if (IS_RTP_MUXER_HTTP_UDP_SOCKET_READY(muxer)) {
		if (IS_RTP_MUXER_HTTP_DEBUG(muxer)) {
			fprintf(stderr,
				"ffmuxer RTP socket already initialized\n");
		}

		return ret;
	}

	for (i = 0; i < RTP_SOCKETS_N; i++) {
		if ((muxer->rtp_s[i] =
		     socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
			ret = -1;
			goto cleanup;
		}

		SET_RTP_SOCKET(muxer, i, RTP_SOCKET_OPEN);

		muxer->rtp_sai_local[i].sin_family = AF_INET;
		muxer->rtp_sai_local[i].sin_port = htons(ps->base_port + 1 + i);
		muxer->rtp_sai_local[i].sin_addr.s_addr = htonl(INADDR_ANY);

		if (bind(muxer->rtp_s[i],
			  (struct sockaddr *)&muxer->rtp_sai_local[i],
			  sizeof(muxer->rtp_sai_local[i])) == -1) {
			ret = -1;
			goto cleanup;
		}

		SET_RTP_SOCKET(muxer, i, RTP_SOCKET_BOUND);

		if (IS_RTP_MUXER_HTTP_DEBUG(muxer)) {
			fprintf(stderr,
				"RTP socket %d initialized on port %d\n",
				muxer->rtp_s[i],
				ntohs(muxer->rtp_sai_local[i].sin_port));
		}
	}

	return ret;

cleanup:
	pstreamer_ffmuxer_udp_sockets_deinit(ps);
err:
	return ret;
}

static int pstreamer_ffmuxer_init(struct pstreamer *ps)
{
	int ret = 0;
	struct rtp_muxer_http *muxer = NULL;

	if (!ps) {
		ret = -1;
		goto err;
	}

	ps->http_muxer = NULL;

	muxer = (struct rtp_muxer_http *) malloc(sizeof(struct rtp_muxer_http));

	if (!muxer) {
		ret = -1;
		goto err_muxer_alloc;
	}

	memset(muxer, 0, sizeof(struct rtp_muxer_http));

	ps->http_muxer = muxer;

	if (pstreamer_ffmuxer_udp_sockets_init(ps) != 0) {
		ret = -1;
		goto err_udp_sockets;
	}

	SET_RTP_MUXER_HTTP_UDP_SOCKET_READY(muxer);

	muxer->rtpb = msg_buffer_init(0 /*Disable Debug*/, "rtp");

	if (!(muxer->rtpb)) {
		ret = -1;
		goto err_rtp_buffer;
	}

	msg_buffer_set_ths_size(muxer->rtpb, RTP_BUFFER_TH1_SIZE,
				RTP_BUFFER_TH2_SIZE);
	msg_buffer_set_start_buf_size_th(muxer->rtpb,
					 RTP_BUFFER_START_BUF_SIZE);
	msg_buffer_set_start_buffering_to_us(muxer->rtpb,
					     RTP_BUFFER_START_BUF_TO_US);

	SET_RTP_MUXER_HTTP_RTP_BUFFER_READY(muxer);

	muxer->httpb = msg_buffer_init(0 /*Disable Debug*/, "http");

	if (!(muxer->httpb)) {
		ret = -1;
		goto err_http_buffer;
	}

	msg_buffer_set_ths_size(muxer->httpb, RTP_BUFFER_TH1_SIZE,
				RTP_BUFFER_TH2_SIZE);
	msg_buffer_set_start_buf_size_th(muxer->httpb,
					 HTTP_BUFFER_START_BUF_SIZE);
	msg_buffer_set_start_buffering_to_us(muxer->httpb,
					     HTTP_BUFFER_START_BUF_TO_US);

	SET_RTP_MUXER_HTTP_HTTP_BUFFER_READY(muxer);

	muxer->rtp_buffer_next_perc_display = 10;
	muxer->http_buffer_next_perc_display = 10;
	muxer->http_root = NULL;

	return ret;

err_http_buffer:
	msg_buffer_destroy(&(muxer->rtpb));
err_rtp_buffer:
err_udp_sockets:
	free(muxer);
err_muxer_alloc:
err:
	return ret;
}

static int pstreamer_ffmuxer_deinit(struct pstreamer *ps)
{
	struct rtp_muxer_http *muxer = NULL;
	int ret = 0;

	if (!ps) {
		ret = -1;
		goto err;
	}

	muxer = ps->http_muxer;

	if (muxer) {
		if (IS_RTP_MUXER_HTTP_FFMUXER_READY(muxer)) {
			ffmuxer_close(&(muxer->ffcontext));
		}

		if (IS_RTP_MUXER_HTTP_HTTP_BUFFER_READY(muxer)) {
			msg_buffer_destroy(&(muxer->httpb));
		}

		if (IS_RTP_MUXER_HTTP_RTP_BUFFER_READY(muxer)) {
			msg_buffer_destroy(&(muxer->rtpb));
		}

		if (IS_RTP_MUXER_HTTP_UDP_SOCKET_READY(muxer)) {
			pstreamer_ffmuxer_udp_sockets_deinit(ps);
		}

		free(muxer);
		ps->http_muxer = NULL;
	}

	return ret;

err:
	return ret;
}

int8_t pstreamer_init(struct pstreamer * ps, const char * rtp_dst_ip, const char * opts)
/* we assume source_ip and source_port are valid strings */
{
	char config[MAX_PSINSTANCE_CONFIG_LENGTH];
	char * fmt = "port=%d,dechunkiser=rtp,base=%d,addr=%s";
	int8_t ret = 0;
	int count;

	count = snprintf(config, MAX_PSINSTANCE_CONFIG_LENGTH, fmt, ps->base_port, ps->base_port+1, rtp_dst_ip);
	if (opts && (size_t)(MAX_PSINSTANCE_CONFIG_LENGTH - count) > strlen(opts))
		snprintf(config + count, MAX_PSINSTANCE_CONFIG_LENGTH - count, ",%s", opts);
	ps->psc = psinstance_create(ps->source_ip, ps->source_port, config);

	ps->topology_task = NULL;
	ps->offer_task = NULL;
	ps->msg_task = NULL;
	ps->muxer_task = NULL;
	ps->tm = NULL;
	ps->topology_interval = 400;

	if (!(ps->psc)) {
		ret = -1;
		goto err;
	}

	if (pstreamer_ffmuxer_init(ps) != 0) {
		ret = -1;
		fprintf(stderr, "ERROR: pstreamer_ffmuxer_init()\n");
		goto err;
	}

	return ret;

err:
	return ret;
}

int8_t pstreamer_deinit(struct pstreamer * ps)
{
	if (ps->topology_task)
		task_manager_destroy_task(ps->tm, &(ps->topology_task));
	if (ps->offer_task)
		task_manager_destroy_task(ps->tm, &(ps->offer_task));
	if (ps->msg_task)
		task_manager_destroy_task(ps->tm, &(ps->msg_task));
	if (ps->muxer_task)
		task_manager_destroy_task(ps->tm, &(ps->muxer_task));
	pstreamer_ffmuxer_deinit(ps);
	psinstance_destroy(&(ps->psc));
	return 0;
}

int8_t pstreamer_cmp(const void * v1, const void * v2)
{
	const struct pstreamer *ps1, *ps2;

	ps1 = (struct pstreamer *) v1;
	ps2 = (struct pstreamer *) v2;

	if (ps1 && ps2)
	{
		return strcmp(ps1->id, ps2->id);
	}
	return 0;
}

char * pstreamer_to_json(const struct pstreamer * ps)
{
	char fmt[] = "{\"id\":\"%s\",\"source_ip\":\"%s\",\"source_port\":\"%d\"}";
	size_t reslength;
	char * res = NULL;

	if (ps)
	{
		reslength = strlen(fmt) - 2*3 + PSID_LENGTH + MAX_IPADDR_LENGTH + MAX_PORT_LENGTH + 1;
		res = malloc(reslength * sizeof(char));
		sprintf(res, fmt, ps->id, ps->source_ip, ps->source_port);
	}
	return res;
}

struct pstreamer_manager * pstreamer_manager_new(uint16_t starting_port)
{
	struct pstreamer_manager * psm = NULL;

	psm = malloc(sizeof(struct pstreamer_manager));
	psm->streamers = ord_set_new(1, pstreamer_cmp);
	/* VLC players assume RTP port numbers are even numbers, we allocate RTP ports starting from starting_port + 1
	 * (as the first one is the pstreamer port). So starting_port must be an odd number */
	psm->initial_streaming_port = (starting_port % 2 == 1) ? starting_port : starting_port + 1;
	psm->streamer_opts = NULL;

	return psm;
}

void pstreamer_manager_remove_orphans(struct pstreamer_manager * psm, time_t interval)
{
	struct ord_set * orphans;
	struct pstreamer * ps;
	const void * iter;
	time_t now;

	now = time(NULL);
	orphans = ord_set_new(10, ord_set_dummy_cmp);
	ord_set_for_each(iter, psm->streamers)
	{
		ps = (struct pstreamer *) iter;
		if (now - ps->last_beat > interval)
			ord_set_insert(orphans, (void *) iter, 0);
	}
	ord_set_for_each(iter, orphans)
	{
		ps = (struct pstreamer *) iter;
		debug("Destroying inactive pstreamer instance %s\n", ps->id);
		pstreamer_manager_destroy_streamer(psm, ps);
	}
	ord_set_destroy(&orphans, 0);
}

void pstreamer_manager_destroy(struct pstreamer_manager ** psm)
{
	const void * ps = NULL;
	
	if (psm && *psm)
	{
		ord_set_for_each(ps, (*psm)->streamers)
			pstreamer_deinit((struct pstreamer *)ps);

		ord_set_destroy(&((*psm)->streamers), 1);
		if((*psm)->streamer_opts)
		{
			free((*psm)->streamer_opts);
			(*psm)->streamer_opts = NULL;
		}
		free(*psm);
		*psm = NULL;
	}
}

void pstreamer_touch(struct pstreamer *ps)
{
	ps->last_beat = time(NULL);
}

int8_t pstreamer_schedule_tasks(struct pstreamer *ps, struct task_manager * tm)
{
	if (ps && tm && ps->psc)
	{
		ps->tm = tm;
		ps->topology_task = task_manager_new_task(tm, NULL, pstreamer_topology_task_callback, ps->topology_interval, ps->psc);
		ps->offer_task = task_manager_new_task(tm, pstreamer_offer_task_reinit, pstreamer_offer_task_callback, psinstance_offer_interval(ps->psc), ps->psc); 
		ps->msg_task = task_manager_new_task(tm, pstreamer_msg_handling_task_reinit, pstreamer_msg_handling_task_callback, 1000, ps->psc);
		ps->muxer_task = task_manager_new_task(tm,
					pstreamer_muxer_task_reinit,
					pstreamer_muxer_task_callback,
					(MSG_BUFFER_TH1_TO_US / 1000) / 2,
					ps);
	}
	return 0;
}

uint16_t assign_streaming_ports(struct pstreamer_manager *psm)
{
	const struct pstreamer * ps = NULL;
	const void * iter = NULL;
	uint16_t base_port;
	uint8_t found = 1;

	base_port = psm->initial_streaming_port;
	while (found==1)
	{
		found = 0;
		ord_set_for_each(iter, psm->streamers)
		{
			ps = (const struct pstreamer *) iter;
			if (ps->base_port == base_port)
				found = 1;
		}
		if (found)
			base_port += 6;  /* we allocate 5 port numbers per each streamer; the first is the pstreamer port, the following 4 are the RTP ports (two
					    for audio and two for video). We add the sixth one for compatibility with the VLC players which expect the RTP base
					    port numbers to be even (base_port is kept odd) */
	}
	return base_port;
}

const struct pstreamer * pstreamer_manager_get_streamer(const struct pstreamer_manager *psm, const char * id)
{
	struct pstreamer * ps = NULL;
	const void * ptr = NULL;

	ps = malloc(sizeof(struct pstreamer));
	strncpy(ps->id, id, PSID_LENGTH);
	ptr = ord_set_find(psm->streamers, (const void *) ps);

	free(ps);
	return (const struct pstreamer*) ptr;
}

const struct pstreamer * pstreamer_manager_create_streamer(struct pstreamer_manager * psm, const char * source_ip, const char * source_port, const char * id, const char * rtp_dst_ip)
{
	struct pstreamer * ps = NULL;
	const void * ptr = NULL;

	if (psm && source_ip && source_port && id && rtp_dst_ip)
	{
		ps = malloc(sizeof(struct pstreamer));
		strncpy(ps->source_ip, source_ip, MAX_IPADDR_LENGTH);
		ps->source_port = atoi(source_port);
		strncpy(ps->id, id, PSID_LENGTH);
		ps->base_port = assign_streaming_ports(psm); 
		ptr = ord_set_find(psm->streamers, (const void *) ps);

		if (ptr) {
			goto err_ord_set;
		}

		pstreamer_touch(ps);
		if (pstreamer_init(ps, rtp_dst_ip, psm->streamer_opts) != 0) {
			goto err_pstreamer_init;
		}
		ord_set_insert(psm->streamers, ps, 0);
	}

	return ps;

err_pstreamer_init:
err_ord_set:
	free(ps);
	ps = NULL;
	return ps;
}

uint8_t pstreamer_manager_destroy_streamer(struct pstreamer_manager *psm, const struct pstreamer *ps)
{
	uint8_t res = 1;

	if (psm && ps)
	{
		pstreamer_deinit((struct pstreamer *)ps);
		res = ord_set_remove(psm->streamers, ps, 1);
	}

	return res;
}

const char * pstreamer_id(const struct pstreamer * ps)
{
	if (ps)
		return ps->id;
	return NULL;
}

uint16_t pstreamer_base_port(const struct pstreamer * ps)
{
	if (ps)
		return ps->base_port;
	return 0;
}

int8_t pstreamer_manager_set_streamer_options(struct pstreamer_manager *psm, const char * opts)
{
	int8_t res = -1;
	
	if (psm && opts)
	{
		if(psm->streamer_opts)
		{
			free(psm->streamer_opts);
			psm->streamer_opts = NULL;
		}
		psm->streamer_opts = strdup(opts);
		res = 0;
	}

	return res;
}

void pstreamer_set_ffmuxer_http_connection(const struct pstreamer *ps,
					   struct mg_connection *nc)
{
	ps->http_muxer->nc = nc;
	SET_RTP_MUXER_HTTP_HTTP_MGCONN_READY(ps->http_muxer);
}

int pstreamer_register_ffmuxer_rtp_fds(const struct pstreamer * ps,
			       fd_register_f func, void *handler)
{
	int i;

	if (!IS_RTP_MUXER_HTTP_UDP_SOCKET_READY(ps->http_muxer)) {
		return 0;
	}

	for (i = 0; i < RTP_SOCKETS_N; i++) {
		func(handler, ps->http_muxer->rtp_s[i], 'r');
	}
	return 0;
}

timeout pstreamer_get_ffmuxer_http_timeout(const struct pstreamer *ps)
{
	uint32_t ufps;
	struct timeval last_http_pop, tv_now;
	long int diff;

	if (!IS_RTP_MUXER_HTTP_HTTP_BUFFER_READY(ps->http_muxer) ||
	    !IS_RTP_MUXER_HTTP_FFMUXER_READY(ps->http_muxer)) {
		return (timeout) ((MSG_BUFFER_TH1_TO_US / 1000) / 2);
	}

	ufps = ffmuxer_get_fps_us(ps->http_muxer->ffcontext);

	if (ufps == 0) {
		ufps = 30000;
	}

	msg_buffer_get_last_pop(ps->http_muxer->httpb, &last_http_pop);

	if ((last_http_pop.tv_usec == 0) && (last_http_pop.tv_sec == 0)) {
		return (timeout) ((ufps / 1000) / 2);
	}

	gettimeofday(&tv_now, NULL);

	diff = (tv_now.tv_usec + 1000000 * tv_now.tv_sec) -
		(last_http_pop.tv_usec + 1000000 * last_http_pop.tv_sec);

	if ((diff < 0) || (diff > ufps)) {
		return (timeout) ((ufps / 1000) / 2);
	}

	return (timeout) ((diff / 1000) / 2);
}

static int pstreamer_ffmuxer_recv_rtp(const struct pstreamer *ps)
{
	int ret = 0;
	int i, n, recv_len;
	int max_rtp_s = 0;
	struct timeval tv;
	fd_set read_fds;
	struct sockaddr_in sia_remote;
	char buffer[RTP_BUF_SIZE];
	socklen_t sia_remote_len = sizeof(sia_remote);

	FD_ZERO(&read_fds);


	for (i = 0; i < RTP_SOCKETS_N; i++) {
		if (ps->http_muxer->rtp_s[i] > max_rtp_s) {
			max_rtp_s = ps->http_muxer->rtp_s[i];
		}

		FD_SET(ps->http_muxer->rtp_s[i], &read_fds);
	}

	tv.tv_sec = 0;
	tv.tv_usec = 0;
	n = select(max_rtp_s + 1, &read_fds, 0, 0, &tv);

	if (n < 0)
	{
		fprintf(stderr, "ERROR Server : select()\n");
		ret = -1;
		goto err;
	}

	if (n == 0) {
		ret = 0;
		goto timeout;
	}

	if (n > 0) {
		for (i = 0; i < RTP_SOCKETS_N; i++) {
			if (FD_ISSET(ps->http_muxer->rtp_s[i], &read_fds)) {

				if (IS_RTP_MUXER_HTTP_DEBUG(ps->http_muxer)) {
					fprintf(stdout,
						"RTP Socket %d "
						"ready to receive (port %d)\n",
						ps->http_muxer->rtp_s[i],
						ntohs(ps->http_muxer->
						      rtp_sai_local[i].
						      sin_port));
				}

				if ((recv_len = recvfrom(
						ps->http_muxer->rtp_s[i],
						buffer, RTP_BUF_SIZE, 0,
						(struct sockaddr *) &sia_remote,
						&sia_remote_len)) == -1) {
					ret = -1;
					goto err;
				}

				if (IS_RTP_MUXER_HTTP_DEBUG(ps->http_muxer)) {
					fprintf(stdout,
						"Received %d "
						"bytes from %s:%d\n",
						recv_len,
						inet_ntoa(sia_remote.sin_addr),
						ntohs(sia_remote.sin_port));
				}

				if ((ret = msg_buffer_push(ps->http_muxer->rtpb,
					buffer, recv_len)) == -1) {
					goto err;
				}
			}
		}
	}

	return ret;

timeout:
	return ret;
err:
	return ret;
}

static void pstreamer_ffmuxer_send_chunk_video(struct pstreamer *ps,
					       const char *buf, int len)
{
	if (len >0) {
		mg_send_http_chunk(ps->http_muxer->nc, buf, len);
	}
}

int pstreamer_handle_ffmuxer(struct pstreamer *ps)
{
	struct rtp_muxer_http *muxer = ps->http_muxer;
	int ret = 0;
	uint32_t rtpb_status = 0;
	uint32_t httpb_status = 0;
	double rtp_buff_perc = 0;
	double http_buff_perc = 0;
	char sdpfile_name[MAX_SDPFILENAME_LENGTH];
	char *muxer_buf;
	void *packet;
	int len;

	if (IS_RTP_MUXER_HTTP_FFMUXER_CLOSED(muxer)) {
		return ret;
	}

	if (!IS_RTP_MUXER_HTTP_UDP_SOCKET_READY(muxer) ||
	    !IS_RTP_MUXER_HTTP_RTP_BUFFER_READY(muxer) ||
	    !IS_RTP_MUXER_HTTP_HTTP_BUFFER_READY(muxer)) {
		ret = -1;
		goto err;
	}

	if (!IS_RTP_MUXER_HTTP_FFMUXER_CLOSING(muxer) &&
	    !IS_RTP_MUXER_HTTP_FFMUXER_CLOSED(muxer)) {
		/* Receive RTP */
		if ((ret = pstreamer_ffmuxer_recv_rtp(ps)) != 0) {
			goto err;
		}
	} /*else {
		Close rtp socket and free resources
	} */

	/* Wait for RTP initial buffering to be complete */
	rtpb_status = msg_buffer_get_status(muxer->rtpb, NULL);
	if ((rtpb_status != MSG_BUFFER_DATA_READY) &&
	    (rtpb_status != MSG_BUFFER_DATA_FLUSHED) &&
	    !IS_RTP_MUXER_HTTP_RTP_START_BUF_COMPLETE(muxer) &&
	    !msg_buffer_is_flushing(muxer->rtpb)) {
		rtp_buff_perc = msg_buffer_get_start_buffering_perc(
					muxer->rtpb);
		if (rtp_buff_perc >=
		    muxer->rtp_buffer_next_perc_display) {
			fprintf(stdout, "%s: RTP buffering %.2f%%\n", ps->id,
				rtp_buff_perc);
			muxer->rtp_buffer_next_perc_display += 10;
		}

		return ret;

	}

	if (!IS_RTP_MUXER_HTTP_RTP_START_BUF_COMPLETE(muxer)) {
		fprintf(stdout, "%s: RTP buffering complete\n",
			pstreamer_id(ps));
	}

	// If rtp is ready and ffmuxer not initialized then initialize it
	if ((rtpb_status == MSG_BUFFER_DATA_READY) ||
	    (rtpb_status == MSG_BUFFER_DATA_FLUSHED)) {

		if (!IS_RTP_MUXER_HTTP_RTP_START_BUF_COMPLETE(muxer)) {
			SET_RTP_MUXER_HTTP_RTP_START_BUF_COMPLETE(muxer);
		}

		if (!IS_RTP_MUXER_HTTP_FFMUXER_READY(muxer)) {
			// Initialize muxer
			snprintf(sdpfile_name, MAX_PATH_LENGTH, "%s/%s.sdp",
				 muxer->http_root, pstreamer_id(ps));

			muxer->ffcontext = ffmuxer_open(sdpfile_name,
						0 /* Disable debug */, ps,
						pstreamer_ffmuxer_read_packet,
						pstreamer_ffmuxer_write_packet);

			if (!(muxer->ffcontext)) {
				/* This is really bad
				 * TODO: at this point we should destroy this
				 * pstreamer instance
				 */
				fprintf(stderr,
					"ERROR (%s): Impossible to allocate "
					"ffmuxer\n", pstreamer_id(ps));
				return -1;
			}

			ffmuxer_set_analysis_duration_and_size(muxer->ffcontext,
				AV_TIME_BASE,
				msg_buffer_get_current_size(muxer->rtpb) / 2);

			SET_RTP_MUXER_HTTP_FFMUXER_READY(muxer);
			fprintf(stdout, "ffmuxer initialized %s\n",
				pstreamer_id(ps));
		}

		if (IS_RTP_MUXER_HTTP_FFMUXER_READY(muxer)) {
			if (!IS_RTP_MUXER_HTTP_FFMUXER_INFO_READY(muxer)) {
				if ((ret = ffmuxer_get_stream_info(
					muxer->ffcontext)) < 0) {
					/* Mmmh, what should we do here? */
					SET_RTP_MUXER_HTTP_FFMUXER_CLOSED(
								muxer);
					fprintf(stderr,
						"ERROR (%s): Impossible to "
						"get stream info\n",
						pstreamer_id(ps));
					goto err;
				}
			}

			SET_RTP_MUXER_HTTP_FFMUXER_INFO_READY(muxer);

			if (IS_RTP_MUXER_HTTP_FFMUXER_INFO_READY(muxer) &&
			    !IS_RTP_MUXER_HTTP_FFMUXER_OUTPUT_READY(muxer)) {
				uint32_t ufps;
				if ((ret = ffmuxer_output_stream_init(
					muxer->ffcontext)) == -1) {
					SET_RTP_MUXER_HTTP_FFMUXER_CLOSED(
								muxer);
					goto err;
				}

				ufps = ffmuxer_get_fps_us(muxer->ffcontext);
				if (ufps == 0) {
					SET_RTP_MUXER_HTTP_FFMUXER_CLOSED(
								muxer);
					fprintf(stderr,
						"ERROR (%s): Impossible to "
						"get stream info\n",
						pstreamer_id(ps));
					ret = -1;
					goto err;
				}
				msg_buffer_set_ths_to(muxer->httpb,
						      (ufps + 1) / 2, ufps * 3);
				msg_buffer_set_ths_to(muxer->rtpb,
						      (ufps + 1) / 2, ufps * 3);

				SET_RTP_MUXER_HTTP_FFMUXER_OUTPUT_READY(muxer);
				SET_RTP_MUXER_HTTP_FFMUXER_INIT_COMPLETE(muxer);
			}
		}

		if (IS_RTP_MUXER_HTTP_FFMUXER_INIT_COMPLETE(muxer) &&
		    !IS_RTP_MUXER_HTTP_FFMUXER_CLOSING(muxer)) {

			len = ffmuxer_get_buff(muxer->ffcontext, &muxer_buf);

			if (len > 0) {
				msg_buffer_push(muxer->httpb, muxer_buf, len);
			}

			av_free(muxer_buf);

			if (ffmuxer_read(muxer->ffcontext, &packet) < 0) {
				if (msg_buffer_is_flushing(muxer->rtpb)) {

					SET_RTP_MUXER_HTTP_FFMUXER_CLOSING(
						muxer);

					ffmuxer_finish(muxer->ffcontext);
					len = ffmuxer_get_buff(muxer->ffcontext,
							       &muxer_buf);

					if (len > 0) {
						msg_buffer_push(muxer->httpb,
								muxer_buf, len);
					}

					av_free(muxer_buf);

					fprintf(stdout, "%s: ffmuxer closing\n",
						pstreamer_id(ps));
				}
			} else {
				ffmuxer_write(muxer->ffcontext, packet);
			}
		}
	}

	if (IS_RTP_MUXER_HTTP_FFMUXER_INIT_COMPLETE(muxer) &&
	    !IS_RTP_MUXER_HTTP_FFMUXER_CLOSING(muxer)) {

		httpb_status = msg_buffer_get_status(muxer->httpb, NULL);

		if (!IS_RTP_MUXER_HTTP_HTTP_START_BUF_COMPLETE(muxer) &&
		    (httpb_status != MSG_BUFFER_DATA_READY)) {
			http_buff_perc = msg_buffer_get_start_buffering_perc(
						muxer->httpb);
			if (http_buff_perc >=
			    muxer->http_buffer_next_perc_display) {
				fprintf(stdout,
					"%s: HTTP buffering %.2f%%\n", ps->id,
					http_buff_perc);
				muxer->http_buffer_next_perc_display += 10;
			}

			return ret;

		}

		if (!IS_RTP_MUXER_HTTP_HTTP_START_BUF_COMPLETE(muxer)) {
			fprintf(stdout, "%s: HTTP buffering complete\n",
				pstreamer_id(ps));
		}

		if ((httpb_status == MSG_BUFFER_DATA_READY)) {
			// TODO what to do if we arrive here and httpb_status ==
			// MSG_BUFFER_DATA_FLUSHED

			if (!IS_RTP_MUXER_HTTP_HTTP_START_BUF_COMPLETE(muxer)) {
				SET_RTP_MUXER_HTTP_HTTP_START_BUF_COMPLETE(
					muxer);
			}

			if (IS_RTP_MUXER_HTTP_HTTP_MGCONN_READY(muxer)) {
				len = msg_buffer_pop(muxer->httpb, &muxer_buf);

				pstreamer_ffmuxer_send_chunk_video(ps,
								   muxer_buf,
								   len);

				if (muxer_buf) {
					free(muxer_buf);
				}
			}
		}
	}

	// CLOSING (from line 487 di rtp_http.c)
	if (IS_RTP_MUXER_HTTP_FFMUXER_INIT_COMPLETE(muxer) &&
	    IS_RTP_MUXER_HTTP_FFMUXER_CLOSING(muxer) &&
	    IS_RTP_MUXER_HTTP_HTTP_MGCONN_READY(muxer)) {

		httpb_status = msg_buffer_get_status(muxer->httpb, NULL);

		if (httpb_status != MSG_BUFFER_DATA_FLUSHED) {
			if (httpb_status != MSG_BUFFER_DATA_NOT_READY) {
				len = msg_buffer_pop(muxer->httpb, &muxer_buf);

				pstreamer_ffmuxer_send_chunk_video(ps,
								   muxer_buf,
								   len);

				if (muxer_buf) {
					free(muxer_buf);
				}
			}
		} else {
			SET_RTP_MUXER_HTTP_FFMUXER_CLOSED(muxer);
			/* Send empty chunk, the end of response */
			mg_send_http_chunk(muxer->nc, "", 0);
			fprintf(stdout, "%s: ffmuxer closed\n",
				pstreamer_id(ps));
		}
	}

	// mg_send_http_chunk(nc, "", 0); /* Send empty chunk, the end of response */
	return ret;

err:
	return ret;
}

void pstreamer_set_ffmuxer_http_root(const struct pstreamer *ps,
				     const char *root)
{
	struct rtp_muxer_http *muxer = ps->http_muxer;
	size_t root_len = strlen(root);

	muxer->http_root = malloc(root_len + 1);

	if (!(muxer->http_root)) {
		/* Probably at this point we should just terminate */
		fprintf(stderr,
			"ERROR: impossible to allocate memory in "
			"pstreamer_set_ffmuxer_http_root()\n");
		return;
	}

	strncpy(muxer->http_root, root, root_len);
}

static void pstreamer_ffmuxer_close_http(const struct pstreamer *ps)
{
	SET_RTP_MUXER_HTTP_FFMUXER_CLOSED(ps->http_muxer);
	fprintf(stdout, "%s: ffmuxer http connection closed\n",
		pstreamer_id(ps));
}

void pstreamer_check_closed_connection(struct pstreamer_manager *psm,
				       struct mg_connection *nc)
{
	const struct pstreamer * ps = NULL;
	const void * iter = NULL;
	ord_set_for_each(iter, psm->streamers)
	{
		ps = (const struct pstreamer *) iter;

		/*
		 * Probably this is not the right way to do this.
		 * If for some reason the Mongoose internals decide to relocate
		 * a previously created mc_connection, then this comparison does
		 * not work and peerstreamer-ng will crash when it tries to send
		 * something into the http ffmuxer connection.
		 * FIXME: check mongoose libraries to see what is the right way
		 * to understand iw two mg_connection pointers refer to the same
		 * mg_connection.
		 */
		if (nc == ps->http_muxer->nc) {
			pstreamer_ffmuxer_close_http(ps);
		}
	}
}
