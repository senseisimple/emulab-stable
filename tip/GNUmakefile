CC = gcc -g

# for BSD
#CFLAGS = -O
#LIBS= -lutil

# for Linux
CFLAGS = -O -DLINUX
LIBS= -ldb

OBJS = acu.o acutab.o cmds.o cmdtab.o cu.o hunt.o log.o partab.o \
       remote.o tip.o tipout.o value.o vars.o getcap.o
DESTDIR = /usr/site

all: tip

tip: $(OBJS)
	$(CC) -o tip $(OBJS) $(LIBS)

tip.static: $(OBJS)
	$(CC) -static -o tip.static $(OBJS) $(LIBS)

install:
	install -s -c tip $(DESTDIR)/bin/tip

clean:
	rm -f $(OBJS) tip tip.static
