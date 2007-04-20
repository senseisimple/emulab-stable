#
# EMULAB-COPYRIGHT
# Copyright (c) 2007 University of Utah and the Flux Group.
# All rights reserved.
#
# Standalone makefile for building Flexlab binaries
#

SUBDIRS = libnetmon magent iperfd magent/UDP/UdpServerDir

all:   all-subdirs
clean: clean-subdirs

# How to recursively descend into subdirectories to make general
# targets such as `all'.
%.MAKE:
	@$(MAKE) -C $(dir $@) $(basename $(notdir $@))
%-subdirs: $(addsuffix /%.MAKE,$(SUBDIRS)) ;

#
# Rules for making a distribution
#

# Files and directories to exclude
DISTFILE=flexlab-dist.tar.gz
EXCLUDES=CVS stub *.tar.gz supafly $(DISTFILE) *.core .\#* bgmon *.bak \
	 db doc schedmon iplane parastream .??*
	 
EXCLUDE_TARSTR=$(foreach pat,$(EXCLUDES),--exclude '$(pat)')

dist:
	$(MAKE) clean
	gtar czvf $(DISTFILE) $^ $(EXCLUDE_TARSTR) -C ../ pelab/
