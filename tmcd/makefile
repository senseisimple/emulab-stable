#
# Testbed TMCD client program
#
BOSSNODE        = boss.emulab.net
#BOSSNODE        = golden-gw.ballmoss.com
CFLAGS	       += -O -g -Wall -DUDP -DSTANDALONE -DBOSSNODE='"$(BOSSNODE)"'
SRCDIR          = .
DESTDIR		=
INSTALL_DIR	= $(DESTDIR)/usr/local/etc/emulab
INSTALL		= /usr/bin/install -c 
INSTALL_PROG	= /usr/bin/install -c -m 755
DISTFILES	= makefile decls.h libsetup.pm ssl.c ssl.h tmcc.c
WAFILES		= liblocsetup-linux.pm liblocsetup-freebsd.pm install.sh \
			emulabctl rc.testbed update vnodesetup watchdog \
			reinstall.sh emulabkey
BSDFILES	= setipod
DISTDIR		= /tmp/emulab

#
# For SSL enabled tmcd/tmcc
#
SSLFLAGS = -DWITHSSL 
TMLIBS	+= -lssl -lcrypto
SSLOBJ   = ssl.o

tmcc: tmcc.c decls.h $(SSLOBJ)
	$(CC) $(CFLAGS) $(SSLFLAGS) -static -g -o tmcc $< $(SSLOBJ) $(TMLIBS)

ssl.o:	ssl.c ssl.h decls.h

clean: 
	rm -f *.o core tmcc

force-install:	misc-install script-install bin-install

dir-install:
	-mkdir -p $(INSTALL_DIR)
	cp /dev/null $(INSTALL_DIR)/isrem
	rm /usr/local/etc/testbed
	ln -s emulab /usr/local/etc/testbed

misc-install:	dir-install

bin-install:	dir-install tmcc
	$(INSTALL_PROG) tmcc $(INSTALL_DIR)/tmcc

script-install:	dir-install
	for file in $(WAFILES); do \
		$(INSTALL_PROG) $(SRCDIR)/$$file $(INSTALL_DIR)/$$file; \
	done
	for file in $(BSDFILES); do \
		$(INSTALL_PROG) $(SRCDIR)/$$file $(INSTALL_DIR)/$$file; \
	done
	$(INSTALL_PROG) $(SRCDIR)/libsetup.pm $(INSTALL_DIR)/libsetup.pm
	$(INSTALL) -m 440 $(SRCDIR)/client.pem $(INSTALL_DIR)/client.pem
	$(INSTALL) -m 440 $(SRCDIR)/emulab.pem $(INSTALL_DIR)/emulab.pem

post-install:
	for file in $(WAFILES); do \
		chown emulabman $(INSTALL_DIR)/$$file; \
		chgrp bin $(INSTALL_DIR)/$$file; \
	done
	chown emulabman $(INSTALL_DIR)/libsetup.pm
	chgrp bin $(INSTALL_DIR)/libsetup.pm
	chown root $(INSTALL_DIR)/update
	chmod u+s $(INSTALL_DIR)/update
	chown root $(INSTALL_DIR)/vnodesetup
	chmod u+s $(INSTALL_DIR)/vnodesetup
	chown root /usr/bin/suidperl
	chmod u+s /usr/bin/suidperl
	chown emulabman $(INSTALL_DIR)/client.pem $(INSTALL_DIR)/emulab.pem
	chmod 640 $(INSTALL_DIR)/client.pem $(INSTALL_DIR)/emulab.pem

dist:	trafgen pemfile
	rm -rf $(DISTDIR)
	mkdir $(DISTDIR)
	cp -p $(DISTFILES) $(DISTDIR)
	cp -p $(BSDFILES) $(DISTDIR)
	for file in $(WAFILES); do cp ron/$$file $(DISTDIR); done
	cp /usr/testbed/etc/emulab.pem $(DISTDIR)
	cp /usr/testbed/etc/$(PEMFILE) $(DISTDIR)/client.pem
	cp /usr/local/sbin/vtund $(DISTDIR)
	cp $(TRAFGEN) $(DISTDIR)
	rm -f $(DISTDIR).tar
	tar cf $(DISTDIR).tar -C $(DISTDIR) .

trafgen:
ifndef	TRAFGEN
	@echo "You must define TRAFGEN (path to trafgen binary)."
	@false
endif

pemfile:
ifndef	PEMFILE
	@echo "You must define PEMFILE (name of the pem file)."
	@false
endif

