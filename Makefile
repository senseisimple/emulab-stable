# Testbed wide makefile.

SUBDIRS = lib assign discvr tbsetup db os security

all:		all-subdirs
install:	all
install:	install-subdirs
post-install:
	@$(MAKE) -C tbsetup post-install
	@$(MAKE) -C security post-install

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
