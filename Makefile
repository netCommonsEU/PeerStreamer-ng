SRC=$(wildcard src/*.c)
OBJS=$(SRC:.c=.o)

EXE=peerstreamer-ng

CFLAGS+=-Isrc/ -ILibs/mongoose/
ifdef DEBUG
CFLAGS+=-g -W -Wall -Wno-unused-function -Wno-unused-parameter -O0
else
CFLAGS+=-O6
endif

LIBS+=Libs/mongoose/mongoose.o 
MONGOOSE_OPTS+=-DMG_DISABLE_MQTT -DMG_DISABLE_JSON_RPC -DMG_DISABLE_SOCKETPAIR  -DMG_DISABLE_CGI # -DMG_DISABLE_HTTP_WEBSOCKET
LDFLAGS+=-lm

all: $(EXE)

$(EXE): $(LIBS) $(OBJS) peerstreamer-ng.c
	$(CC) peerstreamer-ng.c -o peerstreamer-ng $(OBJS) $(CFLAGS) $(LIBS) $(LDFLAGS)

%.o: %.c 
	$(CC) $< -o $@ -c $(CFLAGS) 

Libs/mongoose/mongoose.o:
	git submodule init Libs/mongoose/
	git submodule update Libs/mongoose/
	make -C Libs/mongoose/ CFLAGS="$(CFLAGS)" MONGOOSE_OPTS="$(MONGOOSE_OPTS)"

tests:
	make -C Test/  # CFLAGS="$(CFLAGS)"
	Test/run_tests.sh

clean:
	make -C Test/ clean
	make -C Libs/mongoose clean
	rm -f *.o $(EXE) $(OBJS) $(LIBS)

.PHONY: all clean

