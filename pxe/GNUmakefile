
INSTALL_BINDIR	= /usr/testbed/bin
INSTALL		= /usr/bin/install -c

CFLAGS	= -g -DSOLARIS -DHAVE_SOCKADDR_SA_LEN -DUSE_RECVMSG
INCS	= -I//n/moab/z/stoller/oskit-debug/install/include/oskit/boot

all: proxydhcp bootinfo

proxydhcp: proxydhcp.c
	cc $(CFLAGS) -o proxydhcp proxydhcp.c $(LFLAGS)

bootinfo: bootinfo.c
	cc $(CFLAGS) $(INCS) -o bootinfo bootinfo.c $(LFLAGS)

install:	all
	-mkdir $(INSTALL_BINDIR)	
	-mkdir $(INSTALL_BINDIR)/pxe
	$(INSTALL) proxydhcp $(INSTALL_BINDIR)/pxe
	$(INSTALL) -m 444 proxydhcp.conf $(INSTALL_BINDIR)/pxe
	$(INSTALL) -m 755 proxydhcp.restart $(INSTALL_BINDIR)/pxe
	$(INSTALL) bootinfo $(INSTALL_BINDIR)/pxe
	$(INSTALL) -m 755 bootinfo.restart $(INSTALL_BINDIR)/pxe

clean: 
	rm proxydhcp bootinfo

