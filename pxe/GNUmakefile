
INSTALL_BINDIR	= /usr/testbed/bin
INSTALL		= /usr/bin/install -c

CFLAGS	= -g -DSOLARIS -DHAVE_SOCKADDR_SA_LEN -DUSE_RECVMSG
CFLAGS += -DUSE_NULL_DB
INCS	= -I/n/moab/z/mike/flux/install.debug/include

all: proxydhcp bootinfo

proxydhcp: proxydhcp.c
	cc $(CFLAGS) -o proxydhcp proxydhcp.c $(LFLAGS)

bootinfo: bootinfo.c bootinfo_null.c bootinfo_mysql.c
	cc $(CFLAGS) $(INCS) -I/usr/local/include -o bootinfo $? \
		$(LFLAGS) -L/usr/local/lib/mysql -lmysqlclient

testmysql: bootinfo_mysql.c
	cc $(CFLAGS) -DTEST $(INCS) -o testmysql bootinfo_mysql.c $(LFLAGS)

install:	all
	-mkdir $(INSTALL_BINDIR)	
	-mkdir $(INSTALL_BINDIR)/pxe
	$(INSTALL) proxydhcp $(INSTALL_BINDIR)/pxe
	$(INSTALL) -m 444 proxydhcp.conf $(INSTALL_BINDIR)/pxe
	$(INSTALL) -m 755 proxydhcp.restart $(INSTALL_BINDIR)/pxe
	$(INSTALL) bootinfo $(INSTALL_BINDIR)/pxe
	$(INSTALL) -m 755 bootinfo.restart $(INSTALL_BINDIR)/pxe

clean: 
	rm -f proxydhcp bootinfo testmysql

