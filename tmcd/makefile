#
# Testbed TMCD client program
#
BOSSNODE        = boss.emulab.net
CFLAGS	       += -O -g -Wall -DUDP -DSTANDALONE -DBOSSNODE='"$(BOSSNODE)"'
SRCDIR          = .
DESTDIR		=
INSTALL_DIR	= $(DESTDIR)/usr/local/etc/emulab
RC_DIR		= $(DESTDIR)/usr/local/etc/rc.d
OLD_DIR		= $(DESTDIR)/usr/local/etc/testbed
INSTALL		= /usr/bin/install -c 
INSTALL_PROG	= /usr/bin/install -c -m 755
DISTFILES	= makefile decls.h libsetup.pm ssl.c ssl.h tmcc.c
WAFILES		= liblocsetup-linux.pm liblocsetup-freebsd.pm install.sh \
			emulabctl rc.testbed update vnodesetup watchdog \
			reinstall.sh emulabkey cvsup.auth cvsup.sh \
			runcvsup.sh webserver.pl
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

emulab-install:	misc-install script-install bin-install post-install

dir-install:
	$(INSTALL) -d -m 755 -o root -g wheel $(INSTALL_DIR)
	$(INSTALL) -d -m 750 -o root -g wheel $(INSTALL_DIR)/.cvsup
	$(INSTALL) -d -m 755 -o root -g wheel $(DESTDIR)/var/emulab
	$(INSTALL) -d -m 755 -o root -g wheel $(DESTDIR)/var/emulab/sup
	$(INSTALL) -d -m 755 -o root -g wheel $(RC_DIR)
	cp /dev/null $(INSTALL_DIR)/isrem
	if [ ! -L  $(OLD_DIR) ]; then \
		cp -p $(OLD_DIR)/passdb.db /var/tmp; \
		cp -p $(OLD_DIR)/groupdb.db /var/tmp; \
		cp -p $(OLD_DIR)/passdb.db $(INSTALL_DIR); \
		cp -p $(OLD_DIR)/groupdb.db $(INSTALL_DIR); \
	fi     
	-rm -rf $(DESTDIR)/usr/local/etc/testbed
	ln -s emulab $(DESTDIR)/usr/local/etc/testbed
	-rm -f $(INSTALL_DIR)/liblocsetup.pm
	ln -s liblocsetup-freebsd.pm $(INSTALL_DIR)/liblocsetup.pm

misc-install:	dir-install bossnode
	$(INSTALL) bossnode $(INSTALL_DIR)/bossnode
	$(INSTALL) supfile $(INSTALL_DIR)/supfile
	$(INSTALL) -m 440 $(SRCDIR)/client.pem $(INSTALL_DIR)/client.pem
	$(INSTALL) -m 440 $(SRCDIR)/emulab.pem $(INSTALL_DIR)/emulab.pem
	$(INSTALL) -m 440 $(SRCDIR)/cvsup.auth $(INSTALL_DIR)/.cvsup/auth

bin-install:	dir-install tmcc
	$(INSTALL_PROG) tmcc $(INSTALL_DIR)/tmcc
	$(INSTALL_PROG) trafgen $(INSTALL_DIR)/trafgen
	$(INSTALL_PROG) vtund $(INSTALL_DIR)/vtund

script-install:	dir-install
	for file in $(WAFILES); do \
		$(INSTALL_PROG) $(SRCDIR)/$$file $(INSTALL_DIR)/$$file; \
	done
	for file in $(BSDFILES); do \
		$(INSTALL_PROG) $(SRCDIR)/$$file $(INSTALL_DIR)/$$file; \
	done
	$(INSTALL_PROG) $(SRCDIR)/libsetup.pm $(INSTALL_DIR)/libsetup.pm
	rm -f $(RC_DIR)/testbed.sh
	rm -f $(RC_DIR)/emulab.sh
	rm -f $(RC_DIR)/z.emulab.sh
	$(INSTALL_PROG) $(SRCDIR)/rc.testbed $(RC_DIR)/z.emulab.sh
	rm -f $(RC_DIR)/0.cvsup.sh
	$(INSTALL_PROG) $(SRCDIR)/cvsup.sh $(RC_DIR)/0.cvsup.sh

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
	-chown root $(DESTDIR)/usr/bin/suidperl
	-chmod u+s $(DESTDIR)/usr/bin/suidperl
	chown emulabman $(INSTALL_DIR)/client.pem $(INSTALL_DIR)/emulab.pem
	chmod 640 $(INSTALL_DIR)/client.pem $(INSTALL_DIR)/emulab.pem

ron-dist:	dist
	cp /usr/testbed/etc/ronnode.pem $(DISTDIR)/client.pem
	cp ron/supfile.ron $(DISTDIR)/supfile
	rm -f $(DISTDIR)-ron.tar
	tar cf $(DISTDIR)-ron.tar -C $(DISTDIR) .

wa-dist:	dist
	cp /usr/testbed/etc/pcwa.pem $(DISTDIR)/client.pem
	cp ron/supfile.wa $(DISTDIR)/supfile
	rm -f $(DISTDIR)-wa.tar
	tar cf $(DISTDIR)-wa.tar -C $(DISTDIR) .

dist:	trafgen
	rm -rf $(DISTDIR)
	mkdir $(DISTDIR)
	cp -p $(DISTFILES) $(DISTDIR)
	for file in $(WAFILES); do cp ron/$$file $(DISTDIR); done
	for file in $(BSDFILES); do cp freebsd/$$file $(DISTDIR); done
	cp /usr/testbed/etc/emulab.pem $(DISTDIR)
	cp /usr/local/sbin/vtund $(DISTDIR)
	cp $(TRAFGEN) $(DISTDIR)

trafgen:
ifndef	TRAFGEN
	@echo "You must define TRAFGEN (path to trafgen binary)."
	@false
endif

#
# We build the bossnode file from config info
#
bossnode:	makefile
	echo >$@ "$(BOSSNODE)"

