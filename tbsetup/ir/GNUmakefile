#
# Insert Copyright Here.
#
include ../../Makeconf
include ../../GNUmakerules

#
# These get installed with .tcl extensions.
# 
SCRIPTS		= assign.tcl handle_os.tcl extract_tb.tcl libir.tcl \
		  handle_ip.tcl libir.pm

all:

#
# Currently, all the stuff that gets installed in the bin directory
# gets duplicated in lib/tbsetup.
#
INSTALL_LIBTBDIR = $(INSTALL_LIBDIR)/tbsetup/ir

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

