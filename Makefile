mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
current_dir := $(patsubst %/,%,$(dir $(mkfile_path)))

SRC=$(wildcard src/*.c)
OBJS=$(SRC:.c=.o)

EXE=peerstreamer-ng
GRAPES=$(current_dir)/Libs/GRAPES
NET_HELPER=$(current_dir)/Libs/pstreamer/Lib/net_helper

LIB_CFLAGS+=-I$(current_dir)/src/ -I$(current_dir)/Libs/mongoose/ -I$(current_dir)/Libs/pstreamer/include -I$(NET_HELPER)/include -I$(GRAPES)/include -L$(GRAPES)/src -L$(NET_HELPER)/  -L$(current_dir)/Libs/pstreamer/src 
ifdef DEBUG
CFLAGS+=-g -W -Wall -Wno-unused-function -Wno-unused-parameter -O0
else
CFLAGS+=-O6
endif

LIBS+=$(current_dir)/Libs/mongoose/mongoose.o $(GRAPES)/src/libgrapes.a $(current_dir)/Libs/pstreamer/src/libpstreamer.a
MONGOOSE_OPTS+=-DMG_DISABLE_MQTT -DMG_DISABLE_JSON_RPC -DMG_DISABLE_SOCKETPAIR  -DMG_DISABLE_CGI # -DMG_DISABLE_HTTP_WEBSOCKET
LDFLAGS+=  -lpstreamer -lgrapes -lnethelper -lm

all: $(EXE) $(current_dir)/Tools/janus/bin/janus

$(EXE): $(LIBS) $(OBJS) peerstreamer-ng.c
	$(CC) -o peerstreamer-ng  peerstreamer-ng.c $(OBJS) $(current_dir)/Libs/mongoose/mongoose.o $(CFLAGS) $(LIB_CFLAGS) $(LDFLAGS)

%.o: %.c 
	$(CC) $< -o $@ -c $(LIB_CFLAGS) $(CFLAGS) 

$(current_dir)/Libs/mongoose/mongoose.o:
	git submodule init $(current_dir)/Libs/mongoose/
	git submodule update $(current_dir)/Libs/mongoose/
	make -C $(current_dir)/Libs/mongoose/ CFLAGS="$(CFLAGS)" MONGOOSE_OPTS="$(MONGOOSE_OPTS)"

$(GRAPES)/src/libgrapes.a:
	git submodule init $(GRAPES)/
	git submodule update $(GRAPES)/
	make -C $(GRAPES)/ 

$(current_dir)/Libs/pstreamer/src/libpstreamer.a:
	git submodule init $(current_dir)/Libs/pstreamer/
	git submodule update $(current_dir)/Libs/pstreamer/
	CFLAGS="$(CFLAGS) -DLOG_CHUNK -DLOG_SIGNAL" NET_HELPER=$(NET_HELPER) GRAPES=$(GRAPES) make -C $(current_dir)/Libs/pstreamer/ 

$(current_dir)/Tools/janus/bin/janus:
	git submodule init $(current_dir)/Libs/janus-gateway/
	git submodule update $(current_dir)/Libs/janus-gateway/
	cd $(current_dir)/Libs/janus-gateway/ && ./autogen.sh
	cd $(current_dir)/Libs/janus-gateway/ && CFLAGS="-I$(current_dir)/Libs/janus-gateway/Libs/libsrtp/" LIBS=" -lsrtp2 " LDFLAGS="-L$(current_dir)/Libs/janus-gateway/Libs/libsrtp" ./configure --disable-all-plugins --disable-all-transports --disable-all-handlers --enable-rest --disable-turn-rest-api --enable-libsrtp2 --enable-static --prefix=$(current_dir)/Tools/janus --enable-plugin-streaming --enable-plugin-videoroom 
	make -C Libs/janus-gateway/ install

tests:
	NET_HELPER=$(NET_HELPER) GRAPES=$(GRAPES) make -C Test/  # CFLAGS="$(CFLAGS)"
	Test/run_tests.sh

clean:
	rm -f *.o $(EXE) $(OBJS) $(LIBS)
	make -C $(current_dir)/Test/ clean
	make -C $(GRAPES) clean
	NET_HELPER=$(NET_HELPER) make -C $(current_dir)/Libs/pstreamer clean

distclean: clean
	make -C $(current_dir)/Libs/mongoose clean
	rm -rf $(current_dir)/Tools/janus
	make -C $(current_dir)/Libs/janus-gateway distclean

.PHONY: all clean distclean

