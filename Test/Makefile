SRC=$(wildcard *.c)
OBJS=$(SRC:.c=.test)

TARGET_SRC += $(wildcard ../src/*.c) 
TARGET_OBJS=$(TARGET_SRC:.c=.o)

GRAPES?=$(PWD)/../Libs/GRAPES
NET_HELPER?=$(PWD)/../Libs/pstreamer/Lib/net_helper

LIBS+=../Libs/mongoose/mongoose.o $(GRAPES)/src/libgrapes.a ../Libs/pstreamer/src/libpstreamer.a
LDFLAGS+=-lpstreamer -lgrapes -lnethelper -lm
CFLAGS += -g -W -Wall -Wno-unused-function -Wno-unused-parameter -O0 -I../Libs/mongoose/  -I../src -I../Libs/pstreamer/include -I$(GRAPES)/include -I$(NET_HELPER)/include
CFLAGS+=-L$(GRAPES)/src  -L../Libs/pstreamer/src -L$(NET_HELPER)

all: $(LIBS) $(TARGET_SRC) $(TARGET_OBJS) $(OBJS)

Libs/mongoose/mongoose.o:
	git submodule init Libs/mongoose/
	git submodule update Libs/mongoose/
	$(CC) -c -o ../Libs/mongoose/mongoose.o ../Libs/mongoose/mongoose.c $(CFLAGS) -DMG_DISABLE_MQTT -DMG_DISABLE_JSON_RPC -DMG_DISABLE_SOCKETPAIR  -DMG_DISABLE_CGI # -DMG_DISABLE_HTTP_WEBSOCKET

$(GRAPES)/src/libgrapes.a:
	git submodule init $(GRAPES)/
	git submodule update $(GRAPES)/
	make -C $(GRAPES)/ 

../Libs/pstreamer/src/libpstreamer.a:
	git submodule init ../Libs/pstreamer/
	git submodule update ../Libs/pstreamer/
	NET_HELPER=$(NET_HELPER) GRAPES=$(GRAPES) make -C ../Libs/pstreamer/ 

%.test: %.c $(TARGET_OBJS) 
	$(CC) -o $@ $< $(CFLAGS) $(TARGET_OBJS) $(LIBS) $(LDFLAGS)

clean:
	rm -f *.test $(LIBS)

.PHONY: all clean

