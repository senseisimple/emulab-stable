###########################################################################
# Desc: Makefile for building and installing the examples
# Author: Andrew Howard
# Date: Apr 10 2002
# CVS: $Id: Makefile,v 1.3 2004-12-15 07:27:07 johnsond Exp $
###########################################################################

include ../Makefile.opt


all: 
	cd simple && make all

dep:
	cd simple && make dep

clean:
	rm -f *~ *.o
	cd simple && make clean

install:
	$(MKDIR) -p $(INSTALL_EG)
	$(MKDIR) -p $(INSTALL_EG)/simple
#	$(INSTALL) -m 644 simple/*.c simple/Makefile $(INSTALL_EG)/simple/
	$(INSTALL) -m 755 simple/simple $(INSTALL_EG)/simple/
