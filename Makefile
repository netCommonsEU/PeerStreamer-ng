SRC=$(wildcard src/*.c)
OBJS=$(SRC:.c=.o)

EXE=peerstreamer-ng

CFLAGS+=-Isrc/ -I Libs/mongoose/
ifdef DEBUG
CFLAGS+=-g -W -Wall -Wno-unused-function -Wno-unused-parameter 
else
CFLAGS+=-O6
endif

LIBS+=Libs/mongoose/mongoose.o
LDFLAGS+=-lm

all: $(EXE)

$(EXE): mongoose $(OBJS) peerstreamer-ng.c
	$(CC) peerstreamer-ng.c -o peerstreamer-ng $(OBJS) $(CFLAGS) $(LIBS) $(LDFLAGS)

%.o: %.c 
	$(CC) $< -o $@ -c $(CFLAGS) 

mongoose:
	make -C Libs/mongoose/

tests:
	make -C Test/
	Test/run_tests.sh

clean:
	make -C Test/ clean
	make -C Libs/mongoose/ clean
	rm -f *.o $(EXE) $(OBJS)

.PHONY: all clean

