all:	descend

include Makefile.main

descend:
	@for i in $(SUBDIRS); do (cd $$i; $(MAKE)); done

install:
	cd src; ${MAKE} install
	cd include; ${MAKE} install

clean:	
	@for i in $(SUBDIRS); do (cd $$i; $(MAKE) clean); done

