###########################################################################
# Desc: Makefile for documentation
# Author: Andrew Howard
# Date: 5 May 2002
# CVS: $Id: Makefile,v 1.1 2004-12-12 23:36:35 johnsond Exp $
###########################################################################

include ../Makefile.opt

all:
	cd tex && make all && cp mezzanine.pdf ..

clean:
	cd tex && make clean

install:
	$(MKDIR) -p $(INSTALL_DOC)	
	$(INSTALL) -m 644 mezzanine.pdf $(INSTALL_DOC)
