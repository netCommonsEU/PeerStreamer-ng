SRC=$(wildcard src/*.c)
OBJS=$(SRC:.c=.o)

EXE=peerstreamer-ng

CFLAGS+=-Isrc/ -ILibs/mongoose/ -ILibs/pstreamer/include -ILibs/GRAPES/include -LLibs/GRAPES/src  -LLibs/pstreamer/src 
ifdef DEBUG
CFLAGS+=-g -W -Wall -Wno-unused-function -Wno-unused-parameter -O0
else
CFLAGS+=-O6
endif

LIBS+=Libs/mongoose/mongoose.o Libs/GRAPES/src/libgrapes.a Libs/pstreamer/src/libpstreamer.a
MONGOOSE_OPTS+=-DMG_DISABLE_MQTT -DMG_DISABLE_JSON_RPC -DMG_DISABLE_SOCKETPAIR  -DMG_DISABLE_CGI # -DMG_DISABLE_HTTP_WEBSOCKET
LDFLAGS+=-lpstreamer -lgrapes -lm

all: $(EXE) Tools/janus/bin/janus

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

Tools/janus/bin/janus:
	git submodule init Libs/janus-gateway/
	git submodule update Libs/janus-gateway/
	cd $(PWD)/Libs/janus-gateway/ && ./autogen.sh
	cd $(PWD)/Libs/janus-gateway/ && SRTP15X_CFLAGS="-I$(PWD)/Libs/janus-gateway/Libs/libsrtp/include" SRTP15X_LIBS="-L$(PWD)/Libs/janus-gateway/Libs/libsrtp" PKG_CONFIG_PATH=$(PWD)/Libs/janus-gateway/Libs/libsrtp ./configure --disable-all-plugins --disable-all-transports --disable-all-handlers --enable-rest --disable-turn-rest-api --enable-static --prefix=$(PWD)/Tools/janus --enable-plugin-streaming --enable-plugin-videoroom #--enable-libsrtp2
	make -C Libs/janus-gateway/ install

tests:
	make -C Test/  # CFLAGS="$(CFLAGS)"
	Test/run_tests.sh

clean:
	rm -rf Tools/janus
	rm -f *.o $(EXE) $(OBJS) $(LIBS)
	make -C Test/ clean
	make -C Libs/mongoose clean
	make -C Libs/GRAPES clean
	make -C Libs/pstreamer clean
	make -C Libs/janus-gateway distclean

.PHONY: all clean

