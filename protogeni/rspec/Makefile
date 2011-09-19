#
# GENIPUBLIC-COPYRIGHT
# Copyright (c) 2011 University of Utah and the Flux Group.
# All rights reserved.
#

SUBDIRS = 0.1 0.2 2

all: all-subdirs

clean: clean-subdirs

%.MAKE:
	@$(MAKE) -C $(dir $@) $(basename $(notdir $@))

%-subdirs: $(addsuffix /%.MAKE,$(SUBDIRS)) ;

