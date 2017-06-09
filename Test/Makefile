SRC=$(wildcard *.c)
OBJS=$(SRC:.c=.test)

TARGET_SRC += $(wildcard ../src/*.c) 
TARGET_OBJS=$(TARGET_SRC:.c=.o)

LIBS+=../Libs/mongoose/mongoose.o ../Libs/GRAPES/src/libgrapes.a ../Libs/pstreamer/src/libpstreamer.a
LDFLAGS+=-lpstreamer -lgrapes -lm
CFLAGS += -g -W -Wall -Wno-unused-function -Wno-unused-parameter -O0 -I../Libs/mongoose/  -I../src -I../Libs/pstreamer/include -I../Libs/GRAPES/include
CFLAGS+=-L../Libs/GRAPES/src  -L../Libs/pstreamer/src 

all: $(LIBS) $(TARGET_SRC) $(TARGET_OBJS) $(OBJS)

Libs/mongoose/mongoose.o:
	git submodule init Libs/mongoose/
	git submodule update Libs/mongoose/
	$(CC) -c -o ../Libs/mongoose/mongoose.o ../Libs/mongoose/mongoose.c $(CFLAGS) -DMG_DISABLE_MQTT -DMG_DISABLE_JSON_RPC -DMG_DISABLE_SOCKETPAIR  -DMG_DISABLE_CGI # -DMG_DISABLE_HTTP_WEBSOCKET

../Libs/GRAPES/src/libgrapes.a:
	git submodule init ../Libs/GRAPES/
	git submodule update ../Libs/GRAPES/
	make -C ../Libs/GRAPES/ 

../Libs/pstreamer/src/libpstreamer.a:
	git submodule init ../Libs/pstreamer/
	git submodule update ../Libs/pstreamer/
	make -C ../Libs/pstreamer/ 

%.test: %.c $(TARGET_OBJS) 
	$(CC) -o $@ $< $(CFLAGS) $(TARGET_OBJS) $(LIBS) $(LDFLAGS)

clean:
	rm -f *.test $(LIBS)

.PHONY: all clean

