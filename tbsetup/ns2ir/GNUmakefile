#
# Insert Copyright Here.
#
include ../../Makeconf
include ../../GNUmakerules

#
# These get installed with .tcl extensions.
# 
SCRIPTS		= event.tcl parse.tcl stubs.tcl link.tcl postparse \
		  tcl-object.tcl node.tcl sim.tcl lan.tcl

all:

#
# Currently, all the stuff that gets installed in the bin directory
# gets duplicated in lib/tbsetup.
#
INSTALL_LIBTBDIR = $(INSTALL_LIBDIR)/tbsetup/ns2ir

install: $(addprefix $(INSTALL_LIBTBDIR)/, $(SCRIPTS))

#
# Leave these rules here. They should be flushed when we no longer
# dup this stuff.
# 
$(INSTALL_LIBTBDIR)/%: %
	@echo "Installing $<"
	-mkdir -p $(INSTALL_LIBTBDIR)
	$(INSTALL) $< $@

clean:
