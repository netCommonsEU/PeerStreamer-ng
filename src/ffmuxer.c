#include <libavformat/avformat.h>
#include <sys/time.h>

#include "ffmuxer.h"

struct ffmuxer_context {
	AVFormatContext *ifmt_ctx;
	AVFormatContext *ofmt_ctx;
	int *stream_mapping;
	int stream_mapping_size;

	int debug;
};

double ffmuxer_get_fps(struct ffmuxer_context *ctx)
{
	int i;
	double fps = 0;
	int nstreams = ctx->ifmt_ctx->nb_streams;

	for (i = 0; i < nstreams; i++) {
		AVStream *st = ctx->ifmt_ctx->streams[i];

		if (st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			int avfps = st->avg_frame_rate.den &&
				    st->avg_frame_rate.num;

			if (avfps) {
				fps = av_q2d(st->avg_frame_rate);
			}
		}
	}

	return fps;
}

uint32_t ffmuxer_get_fps_us(struct ffmuxer_context *ctx)
{
	uint32_t ufps;
	double fps = ffmuxer_get_fps(ctx);

	if (fps == 0) {
		return 0;
	}

	ufps = (uint32_t) ceil((1 / fps) * 1e6);

	return ufps;
}

void ffmuxer_init()
{
	av_register_all();
	avformat_network_init();
}

void ffmuxer_deinit()
{
	avformat_network_deinit();
}

void *ffmuxer_open(const char *in_name,
		   int debug,
		   void *info,
		   int (*read_packet)(void *opaque,
				      uint8_t *buf, int buf_size),
		   int (*write_packet)(void *opaque,
				       uint8_t *buf, int buf_size))
{
	struct ffmuxer_context *ctx;
	int res;
	AVDictionary *options = NULL;
	uint8_t *avio_ctx_buffer = NULL;
	size_t avio_ctx_buffer_size = AV_CUSTOMIO_BUFSIZE;
	AVIOContext *my_avio_ctx;

	av_dict_set(&options, "protocol_whitelist", "file,rtp,udp", 0);
	av_dict_set(&options, "sdp_flags", "custom_io", 0);
	ctx = malloc(sizeof(struct ffmuxer_context));

	if (!ctx) {
		return NULL;
	}

	memset(ctx, 0, sizeof(struct ffmuxer_context));

	ctx->debug = debug;

	ctx->ifmt_ctx = avformat_alloc_context();

	if (!(ctx->ifmt_ctx)) {
		av_free(ctx);
		return NULL;
	}

	res = avio_open2(&my_avio_ctx, in_name, AVIO_FLAG_READ, NULL, NULL);

	if (res < 0) {
		avformat_free_context(ctx->ifmt_ctx);
		av_free(ctx);
		return NULL;
	}

	ctx->ifmt_ctx->pb = my_avio_ctx;
	res = avformat_open_input(&ctx->ifmt_ctx, NULL, NULL, &options);

	if (res < 0) {
		fprintf(stderr, "Could not open input file '%s'\n", in_name);
		av_free(ctx->ifmt_ctx->pb);
		avformat_free_context(ctx->ifmt_ctx);
		av_free(ctx);
		return NULL;
	}

	avio_close(ctx->ifmt_ctx->pb);

	if (ctx->debug) {
		fprintf(stdout, "#streams: %u\n", ctx->ifmt_ctx->nb_streams);
	}

	avio_ctx_buffer = av_malloc(avio_ctx_buffer_size);
	if (!avio_ctx_buffer) {
		fprintf(stderr, "ERROR: av_malloc()\n");
		av_free(ctx->ifmt_ctx->pb);
		avformat_free_context(ctx->ifmt_ctx);
		av_free(ctx);
		return NULL;
	}

	my_avio_ctx = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size,
					 1, info, read_packet,
					 write_packet, NULL);

	if (!my_avio_ctx) {
		fprintf(stderr, "ERROR: avio_alloc_context()\n");
		av_free(avio_ctx_buffer);
		av_free(ctx->ifmt_ctx->pb);
		avformat_free_context(ctx->ifmt_ctx);
		av_free(ctx);
		return NULL;
	}

	ctx->ifmt_ctx->pb = my_avio_ctx;

	return ctx;
}

int ffmuxer_get_stream_info(struct ffmuxer_context *ctx)
{
	int res;
	double fps;

	res = avformat_find_stream_info(ctx->ifmt_ctx, 0);

	if (res < 0) {
		if (ctx->debug) {
		fprintf(stderr, "Failed to retrieve input stream information");
		}

		return res;
	}

	fps = ffmuxer_get_fps(ctx);
	av_dump_format(ctx->ifmt_ctx, 0, "Input", 0);
	fprintf(stdout, "MUXER: input stream fps: %.2f\n", fps);
	return res;
}

void ffmuxer_set_analysis_duration_and_size(struct ffmuxer_context *ctx,
					    int64_t duration,
					    unsigned int probesize)
{
	ctx->ifmt_ctx->max_analyze_duration = duration;
	ctx->ifmt_ctx->probesize = probesize;
}

int ffmuxer_output_stream_init(struct ffmuxer_context *ctx)
{
	unsigned int i;
	int res;
	unsigned int stream_index = 0;

	avformat_alloc_output_context2(&ctx->ofmt_ctx, NULL, "ismv", NULL);
	if (!ctx->ofmt_ctx) {
		fprintf(stderr, "Cannot create output context\n");
		return -1;
	}

	ctx->stream_mapping_size = ctx->ifmt_ctx->nb_streams;
	ctx->stream_mapping = av_mallocz_array(ctx->stream_mapping_size,
						sizeof(*ctx->stream_mapping));
	if (!ctx->stream_mapping) {
		avformat_free_context(ctx->ofmt_ctx);
		return -1;
	}

	for (i = 0; i < ctx->ifmt_ctx->nb_streams; i++) {
		AVStream *out_stream;
		AVStream *in_stream = ctx->ifmt_ctx->streams[i];
		AVCodecParameters *in_codecpar = in_stream->codecpar;

		if (in_codecpar->codec_type == AVMEDIA_TYPE_AUDIO ||
			in_codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			ctx->stream_mapping[i] = stream_index++;

			out_stream = avformat_new_stream(ctx->ofmt_ctx, NULL);

			if (!out_stream) {
				fprintf(stderr,
					"Cannot allocate output stream\n");
				av_free(ctx->stream_mapping);
				ctx->stream_mapping = NULL;
				avformat_free_context(ctx->ofmt_ctx);
				return -1;
			}

			res = avcodec_parameters_copy(out_stream->codecpar,
						      in_codecpar);

			if (res < 0) {
				fprintf(stderr,
					"Failed to copy codec parameters\n");
				av_free(ctx->stream_mapping);
				ctx->stream_mapping = NULL;
				avformat_free_context(ctx->ofmt_ctx);
				return -1;
			}

			out_stream->codecpar->codec_tag = 0;
		} else {
			ctx->stream_mapping[i] = -1;
		}
	}

	av_dump_format(ctx->ofmt_ctx, 0, "Output", 1);

	res = avio_open_dyn_buf(&ctx->ofmt_ctx->pb);

	if (res < 0) {
		fprintf(stderr, "Cannot open dynamic buffer");
		av_free(ctx->stream_mapping);
		ctx->stream_mapping = NULL;
		avformat_free_context(ctx->ofmt_ctx);
		return -1;
	}

	res = avformat_write_header(ctx->ofmt_ctx, NULL);

	if (res < 0) {
		fprintf(stderr, "Error occurred when opening output file\n");
		av_free(ctx->stream_mapping);
		ctx->stream_mapping = NULL;
		avformat_free_context(ctx->ofmt_ctx);
		return -1;
	}

	return 0;
}

int ffmuxer_get_buff(struct ffmuxer_context *ctx, unsigned char **buff)
{
	int len, res;

	len = avio_close_dyn_buf(ctx->ofmt_ctx->pb, buff);
	res = avio_open_dyn_buf(&ctx->ofmt_ctx->pb);

	if (res < 0) {
		fprintf(stderr, "Cannot open dynamic buffer");
		return -1;
	}

	return len;
}

int ffmuxer_read(struct ffmuxer_context *ctx, void **v)
{
	AVPacket *p;
	int res;

	p = av_malloc(sizeof(AVPacket));
	*v = p;

	res = av_read_frame(ctx->ifmt_ctx, p);

	if (res < 0) {
		if (ctx->debug) {
			fprintf(stderr, "av_read_frame() res %d\n", res);
		}
		return -1;
	}

	if (p->stream_index >= ctx->stream_mapping_size ||
	    ctx->stream_mapping[p->stream_index] < 0) {
		av_packet_unref(p);
		av_free(p);
		return 0;
	}

	return 1;
}

int ffmuxer_write(struct ffmuxer_context *ctx, void *p)
{
	AVStream *in_stream, *out_stream;
	AVPacket *pkt;
	int res;

	pkt = p;

	pkt->stream_index = ctx->stream_mapping[pkt->stream_index];
	out_stream = ctx->ofmt_ctx->streams[pkt->stream_index];
	in_stream  = ctx->ifmt_ctx->streams[pkt->stream_index];

	/* copy packet */
	pkt->pts = av_rescale_q_rnd(pkt->pts,
			in_stream->time_base, out_stream->time_base,
			AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
	pkt->dts = av_rescale_q_rnd(pkt->dts,
			in_stream->time_base, out_stream->time_base,
			AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
	pkt->duration = av_rescale_q(pkt->duration,
				     in_stream->time_base,
				     out_stream->time_base);
	pkt->pos = -1;

	res = av_interleaved_write_frame(ctx->ofmt_ctx, pkt);

	if (res < 0) {
		if (ctx->debug) {
			fprintf(stderr, "Error muxing packet\n");
		}

		return 0;
	}

	av_packet_unref(pkt);
	av_free(pkt);

	return 1;
}

void ffmuxer_finish(struct ffmuxer_context *ctx)
{
	av_write_trailer(ctx->ofmt_ctx);
}

void ffmuxer_close(struct ffmuxer_context **ctx)
{
	av_free((*ctx)->ifmt_ctx->pb->buffer);

	av_free((*ctx)->ifmt_ctx->pb);
	avformat_close_input(&((*ctx)->ifmt_ctx));

	if ((*ctx)->ofmt_ctx) {
		avformat_free_context((*ctx)->ofmt_ctx);
	}

	av_free((*ctx)->stream_mapping);
	(*ctx)->stream_mapping = NULL;

	av_free(*ctx);

	*ctx = NULL;
}
