SRC=$(wildcard src/*.c)
OBJS=$(SRC:.c=.o)

EXE=peerstreamer-ng
GRAPES=Libs/GRAPES
NET_HELPER=Libs/pstreamer/Lib/net_helper

CFLAGS+=-Isrc/ -ILibs/mongoose/ -ILibs/pstreamer/include -I$(NET_HELPER)/include -I$(GRAPES)/include -L$(GRAPES)/src -L$(NET_HELPER)/  -LLibs/pstreamer/src 
ifdef DEBUG
CFLAGS+=-g -W -Wall -Wno-unused-function -Wno-unused-parameter -O0
else
CFLAGS+=-O6
endif

LIBS+=Libs/mongoose/mongoose.o $(GRAPES)/src/libgrapes.a Libs/pstreamer/src/libpstreamer.a
MONGOOSE_OPTS+=-DMG_DISABLE_MQTT -DMG_DISABLE_JSON_RPC -DMG_DISABLE_SOCKETPAIR  -DMG_DISABLE_CGI # -DMG_DISABLE_HTTP_WEBSOCKET
LDFLAGS+=  -lpstreamer -lgrapes -lnethelper -lm

all: $(EXE)

$(EXE): $(LIBS) $(OBJS) peerstreamer-ng.c
	$(CC) -o peerstreamer-ng  peerstreamer-ng.c $(OBJS) Libs/mongoose/mongoose.o $(CFLAGS) $(LDFLAGS)

%.o: %.c 
	$(CC) $< -o $@ -c $(CFLAGS) 

Libs/mongoose/mongoose.o:
	git submodule init Libs/mongoose/
	git submodule update Libs/mongoose/
	make -C Libs/mongoose/ CFLAGS="$(CFLAGS)" MONGOOSE_OPTS="$(MONGOOSE_OPTS)"

$(GRAPES)/src/libgrapes.a:
	git submodule init $(GRAPES)/
	git submodule update $(GRAPES)/
	make -C $(GRAPES)/ 

Libs/pstreamer/src/libpstreamer.a:
	git submodule init Libs/pstreamer/
	git submodule update Libs/pstreamer/
	NET_HELPER=$(PWD)/$(NET_HELPER) GRAPES=$(PWD)/$(GRAPES) make -C Libs/pstreamer/ 

tests:
	NET_HELPER=$(PWD)/$(NET_HELPER) GRAPES=$(PWD)/$(GRAPES) make -C Test/  # CFLAGS="$(CFLAGS)"
	Test/run_tests.sh

clean:
	make -C Test/ clean
	make -C Libs/mongoose clean
	make -C $(GRAPES) clean
	make -C Libs/pstreamer clean
	rm -f *.o $(EXE) $(OBJS) $(LIBS)

.PHONY: all clean

