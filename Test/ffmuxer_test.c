#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <ffmuxer.h>

static int read_packet(void *opaque, uint8_t *buf, int buf_size)
{
	return 0;
}

static int write_packet(void *opaque, uint8_t *buf, int buf_size)
{
	return 0;
}

static int create_sdp(char *fname)
{
	FILE *sdpf = NULL;

	sdpf = fopen(fname, "w");

	if (!sdpf) {
		return -1;
	}

	fprintf(sdpf, "SDP:\n");
	fprintf(sdpf, "v=0\n");
	fprintf(sdpf, "o=- 0 0 IN IP4 127.0.0.1\n");
	fprintf(sdpf, "s=Sintel.2010.720p\n");
	fprintf(sdpf, "t=0 0\n");
	fprintf(sdpf, "a=tool:libavformat 57.83.100\n");
	fprintf(sdpf, "m=video 30000 RTP/AVP 96\n");
	fprintf(sdpf, "c=IN IP4 127.0.0.1\n");
	fprintf(sdpf, "b=AS:1995\n");
	fprintf(sdpf, "a=rtpmap:96 H264/90000\n");
	fprintf(sdpf, "a=fmtp:96 packetization-mode=1; sprop-parameter-sets"
		"=Z01AH+ygKAItgLUBAQFAAAADAEAAAA8jxgxlgA==,aMqBEsg=; "
		"profile-level-id=4D401F\n");
	fprintf(sdpf, "m=audio 30002 RTP/AVP 97\n");
	fprintf(sdpf, "c=IN IP4 127.0.0.1\n");
	fprintf(sdpf, "b=AS:160\n");
	fprintf(sdpf, "a=rtpmap:97 MPEG4-GENERIC/48000/2\n");
	fprintf(sdpf, "a=fmtp:97 profile-level-id=1;mode=AAC-hbr;"
		"sizelength=13;indexlength=3;indexdeltalength=3; "
		"config=119056E500\n");

	fclose(sdpf);

	return 0;
}

void ffmuxer_init_test()
{
	int res;
	struct ffmuxer_context *ctx;
#define SDPF "/tmp/sdp.sdp"

	res = create_sdp(SDPF);
	assert(res == 0);

	ffmuxer_init();

	ctx = ffmuxer_open(SDPF, 0, NULL, read_packet, write_packet);
	assert(ctx != NULL);

	ffmuxer_close(&ctx);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

int main(int argc, char ** argv)
{
	ffmuxer_init_test();
	/* TODO:
	 * Implement the test to actually testing the muxing operation
	 */
	return 0;
}
