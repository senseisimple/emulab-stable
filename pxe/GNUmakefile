#
# Insert Copyright Here.
#
include ../Makeconf
include ../GNUmakerules

DBFLAG	= -DUSE_MYSQL_DB #-DUSE_CFILE_DB #-DUSE_NULL_DB
DBSRC	= bootinfo_null.c bootinfo_cfile.c bootinfo_mysql.c

CFLAGS	= -g -DSOLARIS -DHAVE_SOCKADDR_SA_LEN -DUSE_RECVMSG
INCS	= -I/n/moab/z/mike/flux/install.debug/include
#INCS	= -I/build/oskit-debug/install/include

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

install:	$(INSTALL_BINDIR)/pxe/proxydhcp \
		$(INSTALL_BINDIR)/pxe/bootinfo
	$(INSTALL_DATA) proxydhcp.conf $(INSTALL_BINDIR)/pxe
	$(INSTALL_PROGRAM) proxydhcp.restart $(INSTALL_BINDIR)/pxe
	$(INSTALL_PROGRAM) bootinfo.restart $(INSTALL_BINDIR)/pxe
	-mkdir -p $(INSTALL_DIR)/etc
	$(INSTALL_DATA) bootinfo.conf $(INSTALL_DIR)/etc/bootinfo.conf

$(INSTALL_BINDIR)/pxe/%: %
	@echo "Installing $<"
	-mkdir -p $(INSTALL_BINDIR)/pxe
	$(INSTALL_PROGRAM) $< $@

clean: 
	rm -f proxydhcp bootinfo testmysql

