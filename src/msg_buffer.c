#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "msg_buffer.h"


struct msg_buffer_slot {
	uint8_t *buf;
	uint32_t size;
	int busy;
};

struct msg_buffer {
	struct msg_buffer_slot *slots;
	uint32_t nslots;
	int32_t next_slot_push;
	int32_t next_slot_pop;

	struct timeval last_push;
	struct timeval last_pop;

	uint32_t size;
	uint32_t busy_slots;

	int buffer_started;
	struct timeval buffer_started_tv;
	uint32_t start_buf_to_us;
	uint32_t start_buf_size_th;
	int start_buf_complete;
	int data_ready;
	int slow_mode; /* 0 disabled, 1 use th1 parameters, 2 use th2 parameters */
	uint32_t th1_size;
	uint32_t th1_to;
	uint32_t th2_size;
	uint32_t th2_to;

	uint32_t flush_to_us;
	int flushing;

	uint32_t min_to_us;

	int32_t next_slot_parse;

	int debug;
	char *buf_id;
};

/* Return 1 if the difference (t2 - t1) is negative, otherwise 0.  */
static int msg_buffer_timeval_subtract(struct timeval *result,
				       struct timeval *t2,
				       struct timeval *t1)
{
	long int diff = (t2->tv_usec + 1000000 * t2->tv_sec) -
				(t1->tv_usec + 1000000 * t1->tv_sec);
	result->tv_sec = diff / 1000000;
	result->tv_usec = diff % 1000000;

	return (diff < 0);
}

static int msg_buffer_increase_n_slots(struct msg_buffer *msgb)
{
	uint32_t new_nslots = msgb->nslots << 1;
	uint32_t i;
	int32_t next_slot_copy = msgb->next_slot_pop;

	struct msg_buffer_slot * new_slots = (struct msg_buffer_slot *)
			malloc(sizeof(struct msg_buffer_slot) * new_nslots);

	if (!new_slots) {
		fprintf(stderr,
			"RTP_BUFFER ERROR (%s): new_slots allocation failed\n",
			msgb->buf_id);
		return -1;
	}

	i = 0;
	while (i < msgb->nslots) {
		(new_slots + i)->buf = (msgb->slots + next_slot_copy)->buf;
		(new_slots + i)->size = (msgb->slots + next_slot_copy)->size;
		(new_slots + i)->busy = (msgb->slots + next_slot_copy)->busy;

		i++;

		next_slot_copy = (next_slot_copy + 1) % msgb->nslots;
	}

	free(msgb->slots);

	if (msgb->next_slot_parse != MSG_BUFFER_INVALID_SLOT_IDX) {
		if (msgb->next_slot_parse >= msgb->next_slot_pop) {
			msgb->next_slot_parse = msgb->next_slot_parse -
						msgb->next_slot_pop;
		} else {
			msgb->next_slot_parse =
					(msgb->nslots - msgb->next_slot_pop) +
					msgb->next_slot_parse;
		}
	}

	msgb->slots = new_slots;
	msgb->nslots = new_nslots;
	msgb->next_slot_pop = 0;
	msgb->next_slot_push = i;

	return 0;
}

static void msg_buffer_check_start_buf_to(struct msg_buffer *msgb)
{
	if (msgb->buffer_started && (msgb->start_buf_to_us > 0)) {
		struct timeval tv_now, tv_diff;
		gettimeofday(&tv_now, NULL);
		msg_buffer_timeval_subtract(&tv_diff, &tv_now,
					&(msgb->buffer_started_tv));
		if ((tv_diff.tv_usec + 1000000 * tv_diff.tv_sec) >=
		    msgb->start_buf_to_us) {
			if (msgb->debug) {
				fprintf(stdout,
				    "RTP_BUFFER (%s): initial buffering TO\n",
				    msgb->buf_id);
			}
			msgb->start_buf_complete = 1;
		}
	}
}

struct msg_buffer *msg_buffer_init(int debug, char *id)
{
	struct msg_buffer *msgb = (struct msg_buffer *)
					malloc(sizeof(struct msg_buffer));

	if (!msgb) {
		fprintf(stderr,
			"ERROR: struct msg_buffer memory allocation failed\n");
		goto msgb_err;
	}

	memset(msgb, 0, sizeof(struct msg_buffer));

	if (!id) {
		fprintf(stderr, "RTP_BUFFER ERROR: id parameter is NULL\n");
		goto id_err;
	}

	msgb->buf_id = (char *) malloc(strlen(id) + 1);

	if (!(msgb->buf_id)) {
		fprintf(stderr, "RTP_BUFFER ERROR: buf_id allocation failed\n");
		goto id_err;
	}

	strncpy(msgb->buf_id, id, strlen(id) + 1);

	msgb->slots = (struct msg_buffer_slot *)
				malloc(sizeof(struct msg_buffer_slot) *
				MSG_BUFFER_N_START_SLOT);

	if (!(msgb->slots)) {
		fprintf(stderr, "RTP_BUFFER ERROR (%s): "
			"struct msg_buffer_slots memory allocation failed\n",
			msgb->buf_id);
		goto slots_err;
	}

	memset(msgb->slots, 0,
	       sizeof(struct msg_buffer_slot) * MSG_BUFFER_N_START_SLOT);

	msgb->nslots = MSG_BUFFER_N_START_SLOT;
	msgb->next_slot_push = 0;
	msgb->next_slot_pop = MSG_BUFFER_INVALID_SLOT_IDX;
	gettimeofday(&(msgb->last_push), NULL);
	gettimeofday(&(msgb->last_pop), NULL);
	msgb->size = 0;
	msgb->busy_slots = 0;
	msgb->buffer_started = 0;
	msgb->start_buf_to_us = 0;
	msgb->start_buf_size_th = MSG_BUFFER_START_BUF_SIZE_TH;
	msgb->start_buf_complete = 0;
	msgb->data_ready = 0;
	msgb->slow_mode = MSG_BUFFER_SLOW_MODE_TH2;
	msgb->th1_size = MSG_BUFFER_TH1_SIZE;
	msgb->th1_to = MSG_BUFFER_TH1_TO_US;
	msgb->th2_size = MSG_BUFFER_TH2_SIZE;
	msgb->th2_to = MSG_BUFFER_TH2_TO_US;
	msgb->flush_to_us = MSG_BUFFER_FLUSH_TO_US;
	msgb->flushing = 0;
	msgb->min_to_us = MSG_BUFFER_MIN_TO_US;
	msgb->next_slot_parse = MSG_BUFFER_INVALID_SLOT_IDX;
	msgb->debug = debug;

	return msgb;

slots_err:
	free(msgb->buf_id);
id_err:
	free(msgb);
msgb_err:
	return NULL;
}

void msg_buffer_destroy(struct msg_buffer **msgb)
{
	uint32_t i;

	if (*msgb) {
		for (i = 0; i < (*msgb)->nslots; i++) {
		if (((*msgb)->slots + i)->busy) {
			free(((*msgb)->slots + i)->buf);
			((*msgb)->slots + i)->buf = NULL;
			((*msgb)->slots + i)->size = 0;
			((*msgb)->slots + i)->busy = 0;
		}
	}

		free((*msgb)->slots);
		free((*msgb)->buf_id);
		free((*msgb));

		*msgb = NULL;
	}
}

int msg_buffer_push(struct msg_buffer *msgb, uint8_t *buf, uint32_t size)
{
	int res = 0;
	uint8_t *new_buf = NULL;

	/* If all slots are busy, allocate new ones */
	if ((msgb->busy_slots > 0) &&
	    (msgb->next_slot_push == msgb->next_slot_pop)) {
		if ((res = msg_buffer_increase_n_slots(msgb)) == -1) {
			fprintf(stderr, "RTP_BUFFER ERROR (%s): "
				"msg_buffer_increase_n_slots()\n",
				msgb->buf_id);
			return res;
		}
	}

	/* Copy the new buffer into the next slot */
	new_buf = (uint8_t *) malloc(size);
	if (!new_buf) {
		fprintf(stderr, "RTP_BUFFER ERROR (%s): buffer push failed\n",
			msgb->buf_id);
		res = -1;
		return res;
	}

	memcpy(new_buf, buf, size);
	(msgb->slots + msgb->next_slot_push)->buf = new_buf;
	(msgb->slots + msgb->next_slot_push)->size = size;
	(msgb->slots + msgb->next_slot_push)->busy = 1;

	if (msgb->debug) {
		fprintf(stdout, "RTP_BUFFER (%s): "
			"pushed %d bytes in slot %d (tot. size %u)\n",
			msgb->buf_id, size, msgb->next_slot_push,
			msgb->size + size);
	}

	if (msgb->next_slot_pop == MSG_BUFFER_INVALID_SLOT_IDX) {
		msgb->next_slot_pop = msgb->next_slot_push;
	}

	msgb->next_slot_push = (msgb->next_slot_push + 1) % msgb->nslots;
	msgb->size += size;
	msgb->busy_slots++;
	msgb->flushing = 0;
	gettimeofday(&(msgb->last_push), NULL);

	if (!(msgb->buffer_started)) {
		msgb->buffer_started = 1;
		gettimeofday(&(msgb->buffer_started_tv), NULL);
	}

	if (msgb->size >= msgb->th1_size) {
		msgb->data_ready = 1;
		msgb->slow_mode = MSG_BUFFER_SLOW_MODE_DISABLED;
	}
	/* else if (msgb->size >= msgb->th2_size) {
		msgb->slow_mode = 1;
	} */

	if (!(msgb->start_buf_complete)) {
		if (msgb->size >= msgb->start_buf_size_th) {
			msgb->start_buf_complete = 1;
		} else {
			msg_buffer_check_start_buf_to(msgb);
		}
	}

	return res;
}

int msg_buffer_pop(struct msg_buffer *msgb, uint8_t **buf)
{
	int size = 0;

	if (msgb->next_slot_pop == MSG_BUFFER_INVALID_SLOT_IDX) {
		*buf = NULL;
		return size;
	}

	*buf = (msgb->slots + msgb->next_slot_pop)->buf;
	size = (msgb->slots + msgb->next_slot_pop)->size;

	(msgb->slots + msgb->next_slot_pop)->buf = NULL;
	(msgb->slots + msgb->next_slot_pop)->size = 0;
	(msgb->slots + msgb->next_slot_pop)->busy = 0;

	if (msgb->debug) {
		fprintf(stdout, "RTP_BUFFER (%s): "
			"popped %d bytes in slot %d (tot. size %u)\n",
			msgb->buf_id, size, msgb->next_slot_pop,
			msgb->size - size);
	}

	msgb->next_slot_pop = (msgb->next_slot_pop + 1) % msgb->nslots;
	(msgb->busy_slots)--;
	msgb->size -= size;
	gettimeofday(&(msgb->last_pop), NULL);

	if (msgb->busy_slots == 0) {
		msgb->next_slot_pop = MSG_BUFFER_INVALID_SLOT_IDX;
	}

	if (msgb->size < msgb->th1_size) {
		msgb->data_ready = 0;

		if (msgb->slow_mode != MSG_BUFFER_SLOW_MODE_TH2) {
			msgb->slow_mode = MSG_BUFFER_SLOW_MODE_TH1;
		}
	}

	if (msgb->size < msgb->th2_size) {
		msgb->slow_mode = MSG_BUFFER_SLOW_MODE_TH2;
	}

	return size;
}

int msg_buffer_get_status(struct msg_buffer *msgb, struct timeval *tv)
{
	struct timeval tv_now, tv_flush, tv_diff;
	long to;

	msg_buffer_check_start_buf_to(msgb);

	gettimeofday(&tv_now, NULL);
	msg_buffer_timeval_subtract(&tv_flush, &tv_now, &(msgb->last_push));

	if (!(msgb->flushing) && msgb->start_buf_complete &&
	    ((tv_flush.tv_usec + 1000000 * tv_flush.tv_sec) >
	     msgb->flush_to_us)) {
		fprintf(stdout,
			"RTP_BUFFER (%s): flush (size %u, busy slots %u)\n",
			msgb->buf_id, msgb->size, msgb->busy_slots);
		msgb->flushing = 1;
	}

	if ((msgb->next_slot_pop != MSG_BUFFER_INVALID_SLOT_IDX) &&
	    msgb->flushing) {
		if (tv) {
			tv->tv_sec = 0;
			tv->tv_usec = msgb->min_to_us;
		}
		return MSG_BUFFER_DATA_READY;
	}

	if ((msgb->next_slot_pop == MSG_BUFFER_INVALID_SLOT_IDX) &&
	    (msgb->flushing)) {
		fprintf(stdout,
			"RTP_BUFFER (%s): flush complete\n", msgb->buf_id);
		if (tv) {
			tv->tv_sec = 0;
			tv->tv_usec = msgb->min_to_us;
		}
		return MSG_BUFFER_DATA_FLUSHED;
	}

	msg_buffer_timeval_subtract(&tv_diff, &tv_now, &(msgb->last_pop));

	if (msgb->debug) {
		fprintf(stdout, "RTP_BUFFER (%s): "
			"size %u (busy slots %u), TH2 %u, TH1 %u\n",
			msgb->buf_id, msgb->size, msgb->busy_slots,
			msgb->th2_size, msgb->th1_size);
	}

	if (!(msgb->start_buf_complete) ||
	    (msgb->next_slot_pop == MSG_BUFFER_INVALID_SLOT_IDX)) {

		if (tv) {
			to = msgb->th2_to;
			tv->tv_sec = to / 1000000;
			tv->tv_usec = to % 1000000;
		}

		if (msgb->debug) {
			fprintf(stdout, "RTP_BUFFER (%s): "
				"start buffering incomplete or buffer empty"
				"(size %u)\n", msgb->buf_id, msgb->size);
		}

		if (!(msgb->start_buf_complete)) {
			return MSG_BUFFER_DATA_START_BUF;
		} else {
			return MSG_BUFFER_DATA_NOT_READY;
		}
	}

	if (msgb->data_ready) {
		if (tv) {
			tv->tv_sec = 0;
			tv->tv_usec = msgb->min_to_us;
		}

		if (msgb->debug) {
			fprintf(stdout,
				"RTP_BUFFER (%s): DATA READY\n", msgb->buf_id);
		}

		return MSG_BUFFER_DATA_READY;
	}

	if (msgb->slow_mode == MSG_BUFFER_SLOW_MODE_TH2) {
		if ((tv_diff.tv_usec + 1000000 * tv_diff.tv_sec) >=
		    msgb->th2_to) {
			if (tv) {
				tv->tv_sec = 0;
				tv->tv_usec = msgb->min_to_us;
			}

			if (msgb->debug) {
				fprintf(stdout, "RTP_BUFFER (%s): "
					"DATA READY TIMEOUT TH2\n",
					msgb->buf_id);
			}

			return MSG_BUFFER_DATA_READY;
		}

		if (tv) {
			to = msgb->th2_to - (tv->tv_usec + 1000000 * tv->tv_sec);
			tv->tv_sec = to / 1000000;
			tv->tv_usec = to % 1000000;
		}
	}

	if (msgb->slow_mode == MSG_BUFFER_SLOW_MODE_TH1) {
		if ((tv_diff.tv_usec + 1000000 * tv_diff.tv_sec) >=
		    msgb->th1_to) {
			if (tv) {
				tv->tv_sec = 0;
				tv->tv_usec = msgb->min_to_us;
			}

			if (msgb->debug) {
				fprintf(stdout, "RTP_BUFFER (%s): "
					"DATA READY TIMEOUT TH1\n",
					msgb->buf_id);
			}

			return MSG_BUFFER_DATA_READY;
		}

		if (tv) {
			to = msgb->th1_to -
				(tv->tv_usec + 1000000 * tv->tv_sec);
			tv->tv_sec = to / 1000000;
			tv->tv_usec = to % 1000000;
		}
	}

	if (msgb->debug) {
		fprintf(stdout, "RTP_BUFFER (%s): DATA NOT READY\n",
			msgb->buf_id);
	}

	return MSG_BUFFER_DATA_NOT_READY;
}

int msg_buffer_parse_start(struct msg_buffer *msgb)
{
	if (msgb->next_slot_pop == MSG_BUFFER_INVALID_SLOT_IDX) {
		if (msgb->debug) {
			fprintf(stderr,
				"RTP_BUFFER (%s): msg_buffer_parse_start() "
				"failed (buffer empty)\n",
				msgb->buf_id);
		}
		return -1;
	}

	msgb->next_slot_parse = msgb->next_slot_pop;

	return 0;
}

int msg_buffer_parse_next(struct msg_buffer *msgb, uint8_t **buf)
{
	int size = 0;

	if (msgb->next_slot_pop == MSG_BUFFER_INVALID_SLOT_IDX ||
	    msgb->next_slot_parse == MSG_BUFFER_INVALID_SLOT_IDX) {
		*buf = NULL;
		return size;
	}

	*buf = (msgb->slots + msgb->next_slot_parse)->buf;
	size = (msgb->slots + msgb->next_slot_parse)->size;

	if (msgb->debug) {
		fprintf(stdout, "RTP_BUFFER (%s): parsed %d bytes in slot %d\n",
		    msgb->buf_id, size, msgb->next_slot_parse);
	}

	msgb->next_slot_parse = (msgb->next_slot_parse + 1) % msgb->nslots;

	if (msgb->next_slot_parse == msgb->next_slot_push) {
		msgb->next_slot_parse = MSG_BUFFER_INVALID_SLOT_IDX;
	}

	return size;

}

void msg_buffer_parse_stop(struct msg_buffer *msgb)
{
	msgb->next_slot_parse = MSG_BUFFER_INVALID_SLOT_IDX;
}

uint32_t msg_buffer_get_start_buf_size(struct msg_buffer *msgb)
{
	if (!msgb) {
		return 0;
	}

	return msgb->start_buf_size_th;
}

uint32_t msg_buffer_get_current_size(struct msg_buffer *msgb)
{
	if (!msgb) {
		return 0;
	}

	return msgb->size;
}

void msg_buffer_set_ths_to(struct msg_buffer *msgb,
			   uint32_t th1_to_us,
			   uint32_t th2_to_us)
{
	if (msgb) {
		msgb->th1_to = th1_to_us;
		msgb->th2_to = th2_to_us;
	}
}

void msg_buffer_set_ths_size(struct msg_buffer *msgb,
			     uint32_t th1_size,
			     uint32_t th2_size)
{
	if (msgb) {
		msgb->th1_size = th1_size;
		msgb->th2_size = th2_size;
	}
}

void msg_buffer_set_start_buffering_to_us(struct msg_buffer *msgb,
					    uint32_t to_us)
{
	if (msgb) {
		msgb->start_buf_to_us = to_us;
		msgb->buffer_started = 0;
	}
}

void msg_buffer_set_start_buf_size_th(struct msg_buffer *msgb,
				      uint32_t size_th)
{
	if (msgb) {
		msgb->start_buf_size_th = size_th;
	}
}

void msg_buffer_set_flush_to_us(struct msg_buffer *msgb, uint32_t to_us)
{
	if (msgb) {
		msgb->flush_to_us = to_us;
	}
}

void msg_buffer_start_buf_reinit(struct msg_buffer *msgb, uint32_t to_us)
{
	if (msgb) {
		msgb->flushing = 0;
		msgb->start_buf_complete = 0;
		msgb->buffer_started = 1;
		msgb->start_buf_to_us = to_us;
		gettimeofday(&(msgb->buffer_started_tv), NULL);
	}
}

int msg_buffer_is_flushing(struct msg_buffer *msgb)
{
	if (msgb) {
		if (msgb->flushing) {
			return 1;
		}
	}

	return 0;
}

uint32_t msg_buffer_get_nslots(struct msg_buffer *msgb)
{
	if (msgb) {
		return msgb->nslots;
	}

	return 0;
}
