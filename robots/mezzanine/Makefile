###########################################################################
# Desc: Top-level makefile for Mezzanine
# Author: Andrew Howard
# Date: 21 Mar 2002
# CVS: $Id: Makefile,v 1.1 2004-12-12 23:36:33 johnsond Exp $
###########################################################################

include Makefile.opt

###########################################################################
# Build section
###########################################################################

all: 
	cd rtk2 && make all
	cd libfg && make all
	cd libmezz && make all
	cd mezzanine && make all
	cd mezzcal && make all
	cd examples && make all

dep:
	cd rtk2 && make dep
	cd libfg && make dep
	cd libmezz && make dep
	cd mezzanine && make dep
	cd mezzcal && make dep
	cd examples && make dep

clean:
	rm -f *~ *.o
	cd rtk2 && make clean
	cd libfg && make clean
	cd libmezz && make clean
	cd mezzanine && make clean
	cd mezzcal && make clean
	cd examples && make clean

install:
	echo "Installing Mezzanine to " $(INSTALL_DIR)
	cd mezzanine && make install
	cd libmezz && make install
	cd mezzcal && make install
	cd examples && make install
	cd docs && make install