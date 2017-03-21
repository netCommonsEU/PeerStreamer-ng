SRC=$(wildcard *.c)
OBJS=$(SRC:.c=.test)

TARGET_SRC += $(wildcard ../src/*.c) 
TARGET_OBJS=$(TARGET_SRC:.c=.o)

LIBS=-lm ../Libs/mongoose/mongoose.o
CFLAGS=-g -O0 -I../Libs/mongoose/  -I../src

all: mongoose $(TARGET_SRC) $(TARGET_OBJS) $(OBJS)

mongoose:
	make -C ../Libs/mongoose/

%.test: %.c $(TARGET_OBJS) 
	$(CC) -o $@ $< $(CFLAGS) $(TARGET_OBJS) $(LIBS)

clean:
	rm -f *.test

.PHONY: all clean

