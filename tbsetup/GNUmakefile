SCRIPTS		= mkprojdir_wrapper
SUSCRIPTS	= mkprojdir

INSTALL		= install -c
INSTALL_BINDIR  = /usr/testbed/bin

all:	$(BINS)

install: $(addprefix $(INSTALL_BINDIR)/, $(SCRIPTS)) \
	 $(addprefix $(INSTALL_BINDIR)/, $(SUSCRIPTS))
	@echo "Don't forget to do a post-install as root"

$(INSTALL_BINDIR)/%: %
	@echo "Installing $<"
	-mkdir -p $(INSTALL_BINDIR)
	$(INSTALL) $< $@

post-install: 
	chown root $(INSTALL_BINDIR)/mkprojdir
	chmod u+s $(INSTALL_BINDIR)/mkprojdir

clean:
	rm -f *.o $(BINS) core
