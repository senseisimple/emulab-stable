CC = gcc -g -O2

OBJS = cmds.o cmdtab.o hunt.o partab.o \
       remote.o tip.o value.o vars.o getcap.o

#
# If HAVE_UUCPLOCK is defined you need -lutil for BSD
#
LIBS=

DESTDIR = /usr/testbed

all: tip.static # tip

tip: $(OBJS)
	$(CC) -o tip $(OBJS) $(LIBS)

tip.static: $(OBJS)
	$(CC) -static -o tip.static $(OBJS) $(LIBS)

$(OBJS): tipconf.h tip.h

install: all
#	install -s -c tip $(DESTDIR)/bin/tip
	install -s -c tip.static $(DESTDIR)/bin/ntip

clean:
	rm -f $(OBJS) tip tip.static
