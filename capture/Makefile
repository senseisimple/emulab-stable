DESTDIR=

#
# Define LOG_DROPS to record warnings in syslog whenever chars were dropped
# due to the output device/pty being full.
#
CFLAGS= -g -O2 -DLOG_DROPS

all: capture

capture: capture.c
	cc $(CFLAGS) -o capture capture.c

install: capture
	install -s capture $(DESTDIR)/usr/site/bin

clean: FRC
	rm -f *.o capture core errs Errs

FRC:

