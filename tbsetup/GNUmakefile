#
# Insert Copyright Here.
#
include ../Makeconf
include ../GNUmakerules

SUBDIRS		= checkpass
SCRIPTS		= mkprojdir_wrapper tbdoit tbstopit mkexpdir \
		  delay_setup ifc_setup ifc_filegen \
		  ir2ifc power resetvlans savevlans snmpit tbend \
		  tbprerun tbreport vpower vsnmpit killtip \
		  mkacct-ctrl_wrapper rmprojdir_wrapper
DATAFILES	= default.ifc
SUSCRIPTS	= mkprojdir rmprojdir os_setup mkacct rmacct \
	          mkacct-ctrl rmacct-ctrl ifc_setup os_load

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
	chown root $(INSTALL_BINDIR)/rmprojdir
	chmod u+s $(INSTALL_BINDIR)/rmprojdir
	chown root $(INSTALL_BINDIR)/mkacct-ctrl
	chmod u+s $(INSTALL_BINDIR)/mkacct-ctrl
	chown root $(INSTALL_LIBTBDIR)/mkacct-ctrl
	chmod u+s $(INSTALL_LIBTBDIR)/mkacct-ctrl
	chown root $(INSTALL_BINDIR)/rmacct-ctrl
	chmod u+s $(INSTALL_BINDIR)/rmacct-ctrl
	chown root $(INSTALL_LIBTBDIR)/rmacct-ctrl
	chmod u+s $(INSTALL_LIBTBDIR)/rmacct-ctrl
	chown root $(INSTALL_BINDIR)/mkacct
	chmod u+s $(INSTALL_BINDIR)/mkacct
	chown root $(INSTALL_LIBTBDIR)/mkacct
	chmod u+s $(INSTALL_LIBTBDIR)/mkacct
	chown root $(INSTALL_BINDIR)/rmacct
	chmod u+s $(INSTALL_BINDIR)/rmacct
	chown root $(INSTALL_LIBTBDIR)/rmacct
	chmod u+s $(INSTALL_LIBTBDIR)/rmacct
	chown root $(INSTALL_BINDIR)/os_setup
	chmod u+s $(INSTALL_BINDIR)/os_setup
	chown root $(INSTALL_LIBTBDIR)/os_setup
	chmod u+s $(INSTALL_LIBTBDIR)/os_setup
	chown root $(INSTALL_BINDIR)/os_load
	chmod u+s $(INSTALL_BINDIR)/os_load
	chown root $(INSTALL_LIBTBDIR)/os_load
	chmod u+s $(INSTALL_LIBTBDIR)/os_load
	chown root $(INSTALL_BINDIR)/ifc_setup
	chmod u+s $(INSTALL_BINDIR)/ifc_setup
	chown root $(INSTALL_LIBTBDIR)/ifc_setup
	chmod u+s $(INSTALL_LIBTBDIR)/ifc_setup
	chown root $(INSTALL_BINDIR)/delay_setup
	chmod u+s $(INSTALL_BINDIR)/delay_setup
	chown root $(INSTALL_LIBTBDIR)/delay_setup
	chmod u+s $(INSTALL_LIBTBDIR)/delay_setup

clean:
	rm -f *.o $(BINS) core
