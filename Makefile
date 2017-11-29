SRC=$(wildcard src/*.c)
OBJS=$(SRC:.c=.o)

EXE=peerstreamer-ng

CFLAGS+=-Isrc/ -ILibs/mongoose/ -ILibs/pstreamer/include -ILibs/GRAPES/include -ILibs/ffmpeg/ -LLibs/GRAPES/src  -LLibs/pstreamer/src -LLibs/ffmpeg/libavutil -LLibs/ffmpeg/libavformat -LLibs/ffmpeg/libavcodec -LLibs/ffmpeg/libswresample
ifdef DEBUG
CFLAGS+=-g -W -Wall -Wno-unused-function -Wno-unused-parameter -O0
else
CFLAGS+=-O6
endif

FFMPEG_CONFIG=--disable-cuda --disable-cuvid --disable-programs --disable-doc --disable-debug --disable-logging --disable-avdevice --disable-avfilter --disable-encoders --disable-filters --disable-bsfs --disable-decoders --enable-decoder=aac,h264,vp8,opus,aac_fixed,aac_latm,h264_v4l2m2m --disable-parsers --enable-parser=aac,h264,opus,vp8 --disable-demuxers --enable-demuxer=sdp,rtp,srtp,aac,h264,vp8,opus --disable-muxers --enable-muxer=webm,ismv,mp4,matroska

LIBS+=Libs/mongoose/mongoose.o Libs/GRAPES/src/libgrapes.a Libs/pstreamer/src/libpstreamer.a Libs/ffmpeg/libavformat/libavformat.a
MONGOOSE_OPTS+=-DMG_DISABLE_MQTT -DMG_DISABLE_JSON_RPC -DMG_DISABLE_SOCKETPAIR  -DMG_DISABLE_CGI # -DMG_DISABLE_HTTP_WEBSOCKET
LDFLAGS+=-lpstreamer -lgrapes -lpthread -ldl -lavformat -lavcodec -lswresample -lavutil -lm -lz

all: $(EXE)

$(EXE): $(LIBS) $(OBJS) peerstreamer-ng.c
	$(CC) -o peerstreamer-ng  peerstreamer-ng.c $(OBJS) Libs/mongoose/mongoose.o $(CFLAGS) $(LDFLAGS)

%.o: %.c 
	$(CC) $< -o $@ -c $(CFLAGS) 

Libs/mongoose/mongoose.o:
	git submodule init Libs/mongoose/
	git submodule update Libs/mongoose/
	make -C Libs/mongoose/ CFLAGS="$(CFLAGS)" MONGOOSE_OPTS="$(MONGOOSE_OPTS)"

Libs/GRAPES/src/libgrapes.a:
	git submodule init Libs/GRAPES/
	git submodule update Libs/GRAPES/
	make -C Libs/GRAPES/ 

Libs/pstreamer/src/libpstreamer.a:
	git submodule init Libs/pstreamer/
	git submodule update Libs/pstreamer/
	make -C Libs/pstreamer/ 

Libs/ffmpeg/libavformat/libavformat.a: Libs/ffmpeg
	cd Libs/ffmpeg; ./configure $(FFMPEG_CONFIG)
	make -C Libs/ffmpeg/

Libs/ffmpeg:
	git clone --branch n3.4 https://git.ffmpeg.org/ffmpeg.git Libs/ffmpeg

tests:
	make -C Test/  # CFLAGS="$(CFLAGS)"
	Test/run_tests.sh

clean:
	make -C Test/ clean
	make -C Libs/mongoose clean
	make -C Libs/GRAPES clean
	make -C Libs/pstreamer clean
	make -C Libs/ffmpeg clean
	rm -f *.o $(EXE) $(OBJS) $(LIBS)

.PHONY: all clean

