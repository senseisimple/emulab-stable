#
# Insert Copyright Here.
#
include ../Makeconf
include ../GNUmakerules

BINS		= suexec
SCRIPTS		= paperbag plasticwrap

CFLAGS		= -O -g

all:	$(BINS)

suexec:	suexec.c suexec.h
	$(CC) $(CFLAGS) -o suexec suexec.c

install: $(addprefix $(INSTALL_BINDIR)/, $(BINS))
	@echo "Don't forget to do a post-install as root"

post-install:
	chown root $(INSTALL_BINDIR)/suexec
	chmod u+s $(INSTALL_BINDIR)/suexec

clean:
	rm -f *.o $(BINS) core
