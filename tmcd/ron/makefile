#
# Post install widearea stuff.
#
BOSSNODE        = boss.emulab.net
CFLAGS	       += -O -g -Wall -DUDP -DSTANDALONE -DBOSSNODE='"$(BOSSNODE)"'
SRCDIR          = .
DESTDIR		=
BINDIR		= $(DESTDIR)/usr/local/etc/emulab
OLDDIR		= $(DESTDIR)/usr/local/etc/testbed
ETCDIR		= $(DESTDIR)/etc/emulab
SYSETCDIR	= $(DESTDIR)/etc
JAILDIR		= $(SYSETCDIR)/jail
INSTALL		= /usr/bin/install -c 
INSTALL_PROG	= /usr/bin/install -c -m 755

#
# For SSL enabled tmcd/tmcc
#
SSLFLAGS	= -DWITHSSL 
TMLIBS	       += -lssl -lcrypto
SSLOBJ		= ssl.o

tmcc: tmcc.c decls.h $(SSLOBJ)
	$(CC) $(CFLAGS) $(SSLFLAGS) -static -g -o tmcc $< $(SSLOBJ) $(TMLIBS)

ssl.o:	ssl.c ssl.h decls.h

clean: 
	rm -f *.o core tmcc

pre-install:
	$(INSTALL) -d -m 755 -o root -g wheel $(DESTDIR)/var/emulab
	$(INSTALL) -d -m 755 -o root -g wheel $(DESTDIR)/var/emulab/db
	if [ -e $(OLDDIR) -a ! -L $(OLDDIR) ]; then \
		cp -fp $(OLDDIR)/passdb.db /var/tmp; \
		cp -fp $(OLDDIR)/groupdb.db /var/tmp; \
		mv -f $(OLDDIR)/passdb.db $(DESTDIR)/var/emulab/db; \
		mv -f $(OLDDIR)/groupdb.db $(DESTDIR)/var/emulab/db; \
		-rm -rf $(OLDDIR); \
	elif [ -e $(BINDIR)/passdb.db ]; then \
		cp -fp $(BINDIR)/passdb.db /var/tmp; \
		cp -fp $(BINDIR)/groupdb.db /var/tmp; \
		cp -fp $(BINDIR)/mountdb.db /var/tmp; \
		cp -fp $(BINDIR)/sfsmountdb.db /var/tmp; \
		mv -f $(BINDIR)/passdb.db $(DESTDIR)/var/emulab/db; \
		mv -f $(BINDIR)/groupdb.db $(DESTDIR)/var/emulab/db; \
		mv -f $(BINDIR)/mountdb.db $(DESTDIR)/var/emulab/db; \
		mv -f $(BINDIR)/sfsmountdb.db $(DESTDIR)/var/emulab/db; \
	fi     

post-install:
	$(BINDIR)/mkemuman.sh
	(chown emulabman $(BINDIR) $(BINDIR)/*)
	(chgrp bin $(BINDIR) $(BINDIR)/*)
	(chown emulabman $(ETCDIR) $(ETCDIR)/*)
	(chgrp bin $(ETCDIR) $(ETCDIR)/*)
	chown root $(BINDIR)/update
	chmod u+s $(BINDIR)/update
	chown root $(BINDIR)/vnodesetup
	chmod u+s $(BINDIR)/vnodesetup
	-chown root $(DESTDIR)/usr/bin/suidperl
	-chmod u+s $(DESTDIR)/usr/bin/suidperl
	chmod 640 $(ETCDIR)/client.pem $(ETCDIR)/emulab.pem
	chmod 640 $(JAILDIR)/master.passwd
