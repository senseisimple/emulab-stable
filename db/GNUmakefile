#
# Insert Copyright Here.
#
include ../Makeconf
include ../GNUmakerules

BINSCRIPTS	= avail inuse mac2if nalloc nfree nodeip ptopgen \
		  showgraph tblog
ALLSCRIPTS	= dbupdate usrud grpmemb user checkowner.tcl usrmod \
		  $(BINSCRIPTS)

all:

#
# Currently, all the stuff that gets installed in the bin directory
# gets duplicated in lib/tbsetup.
#
INSTALL_LIBTBDIR = $(INSTALL_LIBDIR)/db

install: $(addprefix $(INSTALL_BINDIR)/, $(BINSCRIPTS)) \
	 $(addprefix $(INSTALL_LIBTBDIR)/, $(ALLSCRIPTS))

#
# Leave these rules here. They should be flushed when we no longer
# dup this stuff.
# 
$(INSTALL_LIBTBDIR)/%: %
	@echo "Installing $<"
	-mkdir -p $(INSTALL_LIBTBDIR)
	$(INSTALL) $< $@

clean:

