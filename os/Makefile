#
# Insert Copyright Here.
#
include ../Makeconf
include ../GNUmakerules

SUBDIRS	= imagezip

.PHONY:	$(SUBDIRS)

all:	$(SUBDIRS)

imagezip:
	@$(MAKE) -C imagezip all

install: 
	@$(MAKE) -C imagezip install	

clean:
	@$(MAKE) -C imagezip clean
