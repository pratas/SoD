BIN = .

CC = gcc
CFLAGS = -g -O3 -Wall -fno-strict-aliasing

LIBS = -lm
DEPS = defs.h

PROGS = $(BIN)/SoD

OBJS = mem.o common.o context.o

all:
	$(MAKE) progs

progs: $(PROGS)

$(BIN)/SoD: SoD.c $(DEPS) $(OBJS)
	$(CC) $(CFLAGS) -o $(BIN)/SoD SoD.c $(OBJS) $(LIBS)

mem.o: mem.c mem.h $(DEPS)
	$(CC) -c $(CFLAGS) mem.c

common.o: common.c common.h $(DEPS)
	$(CC) -c $(CFLAGS) common.c

context.o: context.c context.h $(DEPS)
	$(CC) -c $(CFLAGS) context.c

clean:
	/bin/rm -f *.o $(PROGS)


