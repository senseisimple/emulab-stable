DESTDIR=
CFLAGS= -O -DNBPG=2048

all: capture

capture: capture.c
	cc $(CFLAGS) -o capture capture.c

install: capture
	install -s capture $(DESTDIR)/usr/site/bin

clean: FRC
	rm -f *.o capture core errs Errs

FRC:

