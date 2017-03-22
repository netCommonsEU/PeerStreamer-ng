SRC=$(wildcard src/*.c)
OBJS=$(SRC:.c=.o)

EXE=peerstreamer-ng

CFLAGS+=-Isrc/ -ILibs/mongoose/
ifdef DEBUG
CFLAGS+=-g -W -Wall -Wno-unused-function -Wno-unused-parameter 
else
CFLAGS+=-O6
endif

LIBS+=Libs/mongoose/mongoose.o
LDFLAGS+=-lm

all: $(EXE)

$(EXE): $(LIBS) $(OBJS) peerstreamer-ng.c
	$(CC) peerstreamer-ng.c -o peerstreamer-ng $(OBJS) $(CFLAGS) $(LIBS) $(LDFLAGS)

%.o: %.c 
	$(CC) $< -o $@ -c $(CFLAGS) 

Libs/mongoose/mongoose.o:
	git submodule init Libs/mongoose/
	git submodule update Libs/mongoose/
	$(CC) -c -o Libs/mongoose/mongoose.o Libs/mongoose/mongoose.c $(CFLAGS) -DMG_DISABLE_MQTT -DMG_DISABLE_JSON_RPC -DMG_DISABLE_SOCKETPAIR  -DMG_DISABLE_CGI # -DMG_DISABLE_HTTP_WEBSOCKET

tests:
	make -C Test/
	Test/run_tests.sh

clean:
	make -C Test/ clean
	rm -f *.o $(EXE) $(OBJS) $(LIBS)

.PHONY: all clean

