OBJS=divert.o divert_hash.o
CFLAGS += -Wall  -O2   # -DDEBUG=1
CC=gcc
# XXX - if we autoconf-ify this stuff, then fix this next line.
RANLIB=ranlib

all: libdivert.a divertest

divertest: libdivert.a divertest.o
	$(CC) $@.o -L. -ldivert -o $@

libdivert.a: ${OBJS}
	${AR} cr $@ ${OBJS}
	${RANLIB} $@

install: all
	@echo "The install target is not yet functional."
	@echo "Where should we stick these things?"

clean:
	/bin/rm -rf $(OBJS) divertest.o divertest libdivert.a
