# Testbed wide makefile.

SUBDIRS = lib assign discvr tbsetup db os security

all:		all-subdirs
install:	all
install:	install-subdirs
post-install:
	@$(MAKE) -C tbsetup post-install
	@$(MAKE) -C security post-install

install-ifyoudare: all 
	# XXXThese don't work anymore
	#install -c os/gethostkey.tcl /usr/testbed/bin/gethostkey
	#install -c os/hostkey.tcl /usr/testbed/bin/hostkey
	#install -c os/instimage.exp /usr/testbed/bin/instimage
	#install -c os/key7 /usr/testbed/bin
	#install -c os/key8 /usr/testbed/bin
	#install -c os/oslist.tcl /usr/testbed/bin/oslist
	#install -c os/osnode2db.tcl /usr/testbed/bin/osnode2db
	#install -c os/osnodestatus.tcl /usr/testbed/bin/osnodestatus
	#install -c os/ossane.tcl /usr/testbed/bin/ossane
	#install -c os/osset.tcl /usr/testbed/bin/osset
	#install -c os/osstatus.tcl /usr/testbed/bin/osstatus
	#install -c os/setupmachine.sh /usr/testbed/bin/setupmachine
	#install -c os/imagezip/imagezip /usr/testbed/bin/imagezip

# How to recursively descend into subdirectories to make general
# targets such as `all'.
%.MAKE:
	@$(MAKE) -C $(dir $@) $(basename $(notdir $@))
%-subdirs: $(addsuffix /%.MAKE,$(SUBDIRS)) ;

# By default, make any target by descending into subdirectories.
%: %-subdirs ;

clean:
	@$(MAKE) -C assign clean
	@$(MAKE) -C discvr clean
	@$(MAKE) -C os  clean
	@$(MAKE) -C os/imagezip clean
	@$(MAKE) -C tbsetup/checkpass clean

.PHONY: post-install

# Get rid of a bunch of nasty built-in implicit rules.
.SUFFIXES:
