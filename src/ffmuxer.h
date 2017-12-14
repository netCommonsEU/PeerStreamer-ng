#ifndef FFMUXER_H
#define FFMUXER_H

#include <stdint.h>

#define AV_CUSTOMIO_BUFSIZE  (4096)

struct ffmuxer_context;

/*
 * The module is initialized with ffmuxer_open() and ffmuxer_open()
 *
 * When the read_packet is able to provide a certain amount of data the function
 * ffmuxer_get_stream_info can be called to collect the input stream
 * information.
 *
 * Once the input stream information are ready the function
 * ffmuxer_output_stream_init can be called to initialized the output stream.
 *
 * The actual muxing is performed by calling the functions ffmuxer_get_buff(),
 * ffmuxer_read() and ffmuxer_write() in a loop.
 *
 * When there is no more data available the output stream can be finilized with
 * ffmuxer_finish() and all the resourced can be freed with ffmuxer_close()
 */

/* Initialize/Deinitialize avformat */
void ffmuxer_init();
void ffmuxer_deinit();

/* Open and initialize the input stream */
void *ffmuxer_open(const char *in_name,
		   int debug,
		   void *info,
		   int (*read_packet)(void *opaque,
				      uint8_t *buf, int buf_size),
		   int (*write_packet)(void *opaque,
				       uint8_t *buf, int buf_size));
/* Reads packets from the input streamt to get stream information */

int ffmuxer_get_stream_info(struct ffmuxer_context *ctx);

/* Initialize the output stream */
int ffmuxer_output_stream_init(struct ffmuxer_context *ctx);

/* Write the trailer into the output stream */
void ffmuxer_finish(struct ffmuxer_context *ctx);

/* Close both input and output streams and free resources */
void ffmuxer_close(struct ffmuxer_context **ctx);

/* The following three functions must be called in loop for muxing the input
 * stream into the output stream */
int ffmuxer_get_buff(struct ffmuxer_context *ctx, unsigned char **buff);
int ffmuxer_read(struct ffmuxer_context *ctx, void **v);
int ffmuxer_write(struct ffmuxer_context *ctx, void *p);

/* Get the rounded average duration of a video frame in us */
uint32_t ffmuxer_get_fps_us(struct ffmuxer_context *ctx);
/* Get the frames per second of the video */
double ffmuxer_get_fps(struct ffmuxer_context *ctx);

/* Set the size and the duration of the input stream prove. This function must
 * be used before calling ffmuxer_output_stream_init() */
void ffmuxer_set_analysis_duration_and_size(struct ffmuxer_context *ctx,
					    int64_t duration,
					    unsigned int probesize);

#endif // FFMUXER_H
