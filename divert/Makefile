OBJS=divert.o divert_hash.o
POBJS=divert.po divert_hash.po
CFLAGS += -Wall  -O2   # -DDEBUG=1
CC=gcc
# XXX - if we autoconf-ify this stuff, then fix this next line.
RANLIB=ranlib

all: libdivert.a divertest

divert.po: divert.c
	${CC} -c -pg divert.c -o $@

divert_hash.po: divert_hash.c
	${CC} -c -pg divert_hash.c -o $@

divertest: libdivert.a divertest.o
	$(CC) $@.o -L. -ldivert -o $@

libdivert.a: ${OBJS}
	${AR} cr $@ ${OBJS}
	${RANLIB} $@

libdivert_p.a: ${POBJS}
	${AR} cr $@ ${POBJS}
	${RANLIB} $@

install: all
	@echo "The install target is not yet functional."
	@echo "Where should we stick these things?"

clean:
	/bin/rm -rf $(OBJS) ${POBJS} divertest.o divertest libdivert.a
