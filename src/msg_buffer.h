#ifndef MSG_BUFFER
#define MSG_BUFFER

#include <stdint.h>
#include <sys/time.h>

/*
 * Thresholds sizes and timeouts should be carefully selected depending on the
 * type of content stored in the buffer.
 *
 * In case of audio/video content the values should depend on the stream
 * parameters:
 *
 *  AVStream *st = ic->streams[i]; where ic is an AVFormatContext
 *
 */

#define MSG_BUFFER_START_BUF_SIZE_TH	(0x40000) /* 256KiB */
#define MSG_BUFFER_TH1_SIZE		(0x20000) /* 128KiB */
#define MSG_BUFFER_TH1_TO_US		(33000)   /* 33ms (e.g., 30fps video) */
#define MSG_BUFFER_TH2_SIZE		(0x10000) /* 64KiB */
#define MSG_BUFFER_TH2_TO_US		(66000)   /* 66ms */

#define MSG_BUFFER_FLUSH_TO_US		(300000)  /* 300ms */

#define MSG_BUFFER_MIN_TO_US		(0)      /* 0ms */

#define MSG_BUFFER_N_START_SLOT		(4096)    /* Default # starting slots */

#define MSG_BUFFER_INVALID_SLOT_IDX	(-1)

#define MSG_BUFFER_DATA_START_BUF	(0x4)
#define MSG_BUFFER_DATA_FLUSHED		(0x2)
#define MSG_BUFFER_DATA_READY		(0x1)
#define MSG_BUFFER_DATA_NOT_READY	(0x0)

#define MSG_BUFFER_SLOW_MODE_DISABLED	(0x0)
#define MSG_BUFFER_SLOW_MODE_TH1	(0x1)
#define MSG_BUFFER_SLOW_MODE_TH2	(0x2)

/* msg_buffer_init must be called for initializing the msg_buffer.
 *
 * - debug: 1 for printing verbose debuggin messages, 0 otherwise
 * - id: identifier string for this buffer
 *
 * Return a pointer to a struct msg_buffer that must be passed as parameter to
 * all the other functions listed in this file.
 */
struct msg_buffer *msg_buffer_init(int debug, char *id);

/* msg_buffer_destroy frees the resources used by the msg_buffer.
 *
 * - msgb: pointer to a struct msg_buffer previously initialized with
 *   msg_buffer_init
 */
void msg_buffer_destroy(struct msg_buffer **msgb);

/* Push a new message in the msg_buffer */
int msg_buffer_push(struct msg_buffer *msgb, uint8_t *buf, uint32_t size);
/* Check the status of the buffer.
 *
 * Return:
 * - MSG_BUFFER_DATA_READY: a new message can be popped from the buffer
 * - MSG_BUFFER_DATA_FLUSHED: buffer is now empty after the flush
 * - MSG_BUFFER_DATA_START_BUF: buffer is waiting to reach the initial buffering
 *   threshold
 * - MSG_BUFFER_DATA_NOT_READY: there are no messages to pop from the buffer
 *   */
int msg_buffer_get_status(struct msg_buffer *msgb, struct timeval *tv);
/* Pop a message from the buffer.
 * The caller has the responsibility to free buf.
 */
int msg_buffer_pop(struct msg_buffer *msgb, uint8_t **buf);

/* The following three functions are used to parse the content of msg_buffer
 * (starting from next_slot_pop) without actually removing the slots from the
 * buffer
 *
 * This is useful for parsing the content of the buffer without actually
 * removing the content in order to consume it later.
 *
 * The usage patter is the following:
 * - call msg_buffer_parse_start(), if it returns 0 then
 * - call (also multiple times) msg_buffer_parse_next() for parsing the content
 *   of the buffer
 * - When the parsing is complete, close the transaction with
 *   msg_buffer_parse_stop
 */

/* Return -1 if the msg_buffer is empty, 0 otherwise */
int msg_buffer_parse_start(struct msg_buffer *msgb);
int msg_buffer_parse_next(struct msg_buffer *msgb, uint8_t **buf);
void msg_buffer_parse_stop(struct msg_buffer *msgb);

/* Reinitialize the bootstrap of the buffer.
 * The next data will be ready when the buffer reaches a size greater or equal
 * to the start buffering threshold (configured using
 * msg_buffer_set_start_buf_size_th)
 */
void msg_buffer_start_buf_reinit(struct msg_buffer *msgb, uint32_t to_us);

/* Get/Set functions */

/* return the initial buffering threshold */
uint32_t msg_buffer_get_start_buf_size(struct msg_buffer *msgb);
/* return the current buffer size in byte */
uint32_t msg_buffer_get_current_size(struct msg_buffer *msgb);
/* set the timeouts (us) for threshold1 (TH1) and threshold2 (TH2) */
void msg_buffer_set_ths_to(struct msg_buffer *msgb,
			   uint32_t th1_to_us,
			   uint32_t th2_to_us);
/* set the sizes (bytes) for threshold1 (TH1) and threshold2 (TH2) */
void msg_buffer_set_ths_size(struct msg_buffer *msgb,
			     uint32_t th1_size,
			     uint32_t th2_size);
/* set the timeout (us) for the initial buffering */
void msg_buffer_set_start_buffering_to_us(struct msg_buffer *msgb,
					    uint32_t to_us);
/* set the size (bytes) of the initial buffering
 *
 * msg_buffer_get_status() will return MSG_BUFFER_DATA_NOT_READY until the start
 * buffer size is reached or until the initail buffer timeout expires (whatever
 * comes first)
 */
void msg_buffer_set_start_buf_size_th(struct msg_buffer *msgb,
				      uint32_t size_th);
/* Set the flush timeout for the buffer.
 * The buffer start flushing if the timeout expires.
 * The timeout is reset every time a new message is pushed into the buffer
 */
void msg_buffer_set_flush_to_us(struct msg_buffer *msgb, uint32_t to_us);
/* Return 1 if the msg_buffer is currently flushing, 0 otherwise */
int msg_buffer_is_flushing(struct msg_buffer *msgb);
/* Return the number of slots in the buffer */
uint32_t msg_buffer_get_nslots(struct msg_buffer *msgb);

#endif // MSG_BUFFER
