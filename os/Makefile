#
# Insert Copyright Here.
#
include ../Makeconf
include ../GNUmakerules

CC=gcc
CFLAGS=-O

SCRIPTS		= oslib.tcl

all:

key7: key7.c
	$(CC) $(CFLAGS) -o key7 key7.c

key8: key8.c
	$(CC) $(CFLAGS) -o key8 key8.c

install: $(addprefix $(INSTALL_BINDIR)/, $(SCRIPTS))

clean:
	/bin/rm -f *.o key7 key8
