INSTALL_DIR	= /usr/testbed
INSTALL_BINDIR	= $(INSTALL_DIR)/bin
INSTALL		= /usr/bin/install -c

DBFLAG	= -DUSE_MYSQL_DB #-DUSE_CFILE_DB #-DUSE_NULL_DB
DBSRC	= bootinfo_null.c bootinfo_cfile.c bootinfo_mysql.c

CFLAGS	= -g -DSOLARIS -DHAVE_SOCKADDR_SA_LEN -DUSE_RECVMSG
INCS	= -I/n/moab/z/mike/flux/install.debug/include

all: proxydhcp bootinfo

proxydhcp: proxydhcp.c
	cc $(CFLAGS) -o proxydhcp proxydhcp.c $(LFLAGS)

bootinfo: bootinfo.c $(DBSRC) bootinfo_mysql.c
	cc $(CFLAGS) $(DBFLAG) $(INCS) -I/usr/local/include \
		-o bootinfo bootinfo.c $(DBSRC) \
		$(LFLAGS) -L/usr/local/lib/mysql -lmysqlclient

testmysql: bootinfo_mysql.c
	cc $(CFLAGS) -DUSE_MYSQL_DB -DTEST $(INCS) -I/usr/local/include \
		-o testmysql bootinfo_mysql.c \
		$(LFLAGS) -L/usr/local/lib/mysql -lmysqlclient

testcfile: bootinfo_cfile.c
	cc $(CFLAGS) -DUSE_CFILE_DB -DTEST $(INCS) -o testcfile bootinfo_cfile.c \
		$(LFLAGS)

install:	all
	-mkdir $(INSTALL_BINDIR)	
	-mkdir $(INSTALL_BINDIR)/pxe
	$(INSTALL) proxydhcp $(INSTALL_BINDIR)/pxe
	$(INSTALL) -m 444 proxydhcp.conf $(INSTALL_BINDIR)/pxe
	$(INSTALL) -m 755 proxydhcp.restart $(INSTALL_BINDIR)/pxe
	$(INSTALL) bootinfo $(INSTALL_BINDIR)/pxe
	$(INSTALL) -m 755 bootinfo.restart $(INSTALL_BINDIR)/pxe
	$(INSTALL) -m 444 bootinfo.conf $(INSTALL_DIR)/etc/bootinfo.conf

clean: 
	rm -f proxydhcp bootinfo testmysql

