BINS		= suexec
SCRIPTS		= paperbag plasticwrap

CFLAGS		= -O -g
INSTALL		= install -c
INSTALL_BINDIR  = /usr/testbed/bin

all:	$(BINS)

suexec:	suexec.c suexec.h
	$(CC) $(CFLAGS) -o suexec suexec.c

install: $(addprefix $(INSTALL_BINDIR)/, $(BINS))
	@echo "Don't forget to do a post-install as root"

$(INSTALL_BINDIR)/%: %
	@echo "Installing $<"
	-mkdir -p $(INSTALL_BINDIR)
	$(INSTALL) $< $@

post-install:
	chown root $(INSTALL_BINDIR)/suexec
	chmod u+s $(INSTALL_BINDIR)/suexec

clean:
	rm -f *.o $(BINS) core
