#
# Insert Copyright Here.
#
include ../Makeconf
include ../GNUmakerules

SUBDIRS		= checkpass
SCRIPTS		= mkprojdir_wrapper tbdoit tbstopit mkexpdir \
		  mkacct rmacct delay_setup ifc_setup ifc_filegen \
		  ir2ifc power resetvlans savevlans snmpit tbend \
		  tbprerun tbreport vpower vsnmpit killtip
DATAFILES	= default.ifc
SUSCRIPTS	= mkprojdir

all:	$(BINS) $(SUBDIRS)

.PHONY:	checkpass
checkpass:
	@$(MAKE) -C checkpass all

#
# Currently, all the stuff that gets installed in the bin directory
# gets duplicated in lib/tbsetup.
#
INSTALL_LIBTBDIR = $(INSTALL_LIBDIR)/tbsetup

install: $(addprefix $(INSTALL_BINDIR)/, $(SCRIPTS)) \
	 $(addprefix $(INSTALL_BINDIR)/, $(SUSCRIPTS)) \
	 $(addprefix $(INSTALL_LIBTBDIR)/, $(SCRIPTS)) \
	 $(addprefix $(INSTALL_LIBTBDIR)/, $(DATAFILES)) \
	 subdir-install
	@echo "Don't forget to do a post-install as root"

#
# Automate this part at some point.
# 
subdir-install:
	@$(MAKE) -C checkpass install	
	@$(MAKE) -C ir install	
	@$(MAKE) -C ns2ir install	

#
# Leave these rules here. They should be flushed when we no longer
# dup this stuff.
# 
$(INSTALL_LIBTBDIR)/%: %
	@echo "Installing $<"
	-mkdir -p $(INSTALL_LIBTBDIR)
	$(INSTALL) $< $@

$(INSTALL_LIBTBDIR)/%: %.tcl
	@echo "Installing $<"
	-mkdir -p $(INSTALL_LIBTBDIR)
	$(INSTALL) $< $@

post-install: 
	chown root $(INSTALL_BINDIR)/mkprojdir
	chmod u+s $(INSTALL_BINDIR)/mkprojdir

clean:
	rm -f *.o $(BINS) core
