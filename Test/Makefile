SRC=$(wildcard *.c)
OBJS=$(SRC:.c=.test)

TARGET_SRC += $(wildcard ../src/*.c) 
TARGET_OBJS=$(TARGET_SRC:.c=.o)

LIBS=../Libs/mongoose/mongoose.o
LDFLAGS +=-lm
CFLAGS +=-g -W -Wall -Wno-unused-function -Wno-unused-parameter -O0 -I../Libs/mongoose/  -I../src

all: $(LIBS) $(TARGET_SRC) $(TARGET_OBJS) $(OBJS)

Libs/mongoose/mongoose.o:
	git submodule init Libs/mongoose/
	git submodule update Libs/mongoose/
	$(CC) -c -o Libs/mongoose/mongoose.o Libs/mongoose/mongoose.c $(CFLAGS) -DMG_DISABLE_MQTT -DMG_DISABLE_JSON_RPC -DMG_DISABLE_SOCKETPAIR  -DMG_DISABLE_CGI # -DMG_DISABLE_HTTP_WEBSOCKET

%.test: %.c $(TARGET_OBJS) 
	$(CC) -o $@ $< $(CFLAGS) $(TARGET_OBJS) $(LIBS) $(LDFLAGS)

clean:
	rm -f *.test

.PHONY: all clean

