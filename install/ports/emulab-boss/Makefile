# New ports collection makefile for: emulab-boss
# Date created:         28 January 2003
# Whom:                 ricci@cs.utah.edu
# Updated:              1 October 2009 jwalsh@atl.lmco.com

# Need the major version number to figure out to do.
OSMAJOR!= /usr/bin/uname -r | /usr/bin/sed -e 's/\..*//'
OSMINOR!= /usr/bin/uname -r | /usr/bin/sed -e 's/[0-9][0-9]*\.\([0-9][0-9]*\).*/\1/'

PORTNAME=	emulab-boss
.if ${OSMAJOR} > 6
PORTVERSION=	3.0
.else
.if ${OSMAJOR} > 5
.if ${OSMINOR} >= 3
PORTVERSION=	2.1
.else
PORTVERSION=	2.0
.endif
.else
PORTVERSION=	1.7
.endif
.endif
CATEGORIES=	misc
MASTER_SITES=	#none
DISTFILES=	#none
EXTRACT_ONLY=	#none
COMMENT=	"Meta-port for an Emulab boss node"

MAINTAINER=	testbed-ops@flux.utah.edu

#
# We set this so that the mysql server port doesn't do name resolution
#
.MAKEFLAGS+=	-DSKIP_DNS_CHECK

#
# We must make sure PERL is built with this.
#
.MAKEFLAGS+=	ENABLE_SUIDPERL=yes
.if ${PORTVERSION} == "3.0"
.MAKEFLAGS+=	PERL_VERSION=5.10.0 PERL_VER=5.10.0 PERL_ARCH=mach
.endif

#
# For SWIG, so that it doesn't feel the need to suck in php and apache
# XXX Something is broken on FreeBSD 6.X and SWIG_LANGUAGES='x y z' breaks.
#
.MAKEFLAGS+=	SWIG_LANGUAGES= 
.MAKEFLAGS+=    WANT_SWIG_PERL=yes WANT_SWIG_PYTHON=yes WANT_SWIG_TCL=yes

#
# Have not taken the plunge to PHP5
#
.MAKEFLAGS+=	PHP_VER=4

#
# Various ports need to agree on what version of apache. Make sure this
# agrees with what is in the emulab-ops and emulab-php4 metaports.
#
.MAKEFLAGS+=	-DWITH_APACHE
.MAKEFLAGS+=	APACHE_PORT=www/apache13-modssl

#
# This prevents any ports from trying to run interactively - some, like the
# net-snmp port, ask silly questions without this option.
#
.MAKEFLAGS+=	-DBATCH

#
# XXX not really a build dependency, but needs to get installed before
# any attempt to install mysql50-* ports
#
.if (${PORTVERSION} == "2.1" || ${PORTVERSION} == "3.0")
BUILD_DEPENDS=	\
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/Mysql.pm:${PORTSDIR}/misc/p5-DBD-mysql50-old
.else
BUILD_DEPENDS=	\
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/Mysql.pm:${PORTSDIR}/databases/p5-DBD-mysql50
.endif

LIB_DEPENDS= gd.4:${PORTSDIR}/graphics/gd \
	lthread.[236]:${PORTSDIR}/devel/linuxthreads \
	mhash.2:${PORTSDIR}/security/mhash \
	mysqlclient.15:${PORTSDIR}/databases/mysql50-client \
	snmp.4:${PORTSDIR}/net-mgmt/net-snmp4 \
	ulsshxmlrpcpp.0:${PORTSDIR}/misc/ulsshxmlrpcpp

#
# Until/unless the pubsub Makefile is changed, we must include libelvin.
# This is because the pubsub libraries have to be built with ELVIN_COMPAT
# to talk to pubsub on ops, and building with compat means that the gateway
# will be built.  Building that requires elvin libraries...
#
.if defined(ELVIN_COMPAT)
LIB_DEPENDS+= vin4.2:${PORTSDIR}/misc/libelvin \
	pubsub.1:${PORTSDIR}/misc/emulab-pubsub-elvincompat
.else
LIB_DEPENDS+= pubsub.1:${PORTSDIR}/misc/emulab-pubsub
.endif

RUN_DEPENDS=	\
	${LOCALBASE}/etc/apache/ssl.crl:${PORTSDIR}/www/apache13-modssl \
	mod_auth_mysql>1:${PORTSDIR}/www/mod_auth_mysql \
	fping:${PORTSDIR}/net/fping \
	rsync:${PORTSDIR}/net/rsync \
	neato:${PORTSDIR}/graphics/graphviz \
        swish-e:${PORTSDIR}/www/swish-e \
	${LOCALBASE}/libexec/mysqld:${PORTSDIR}/databases/mysql50-server \
	anytopnm:${PORTSDIR}/graphics/netpbm \
	tclsh8.4:${PORTSDIR}/lang/tcl84 \
	otclsh:${PORTSDIR}/lang/otcl \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/GD.pm:${PORTSDIR}/misc/p5-GD-old \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/SNMP.pm:${PORTSDIR}/net-mgmt/p5-SNMP \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/SNMP_Session.pm:${PORTSDIR}/net-mgmt/p5-SNMP_Session \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/BSD/Resource.pm:${PORTSDIR}/devel/p5-BSD-Resource \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/Time/HiRes.pm:${PORTSDIR}/devel/p5-Time-HiRes \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/MD5.pm:${PORTSDIR}/security/p5-MD5 \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/IO/Pty.pm:${PORTSDIR}/devel/p5-IO-Tty \
	rpm:${PORTSDIR}/archivers/rpm \
	xvcg:${PORTSDIR}/graphics/vcg \
	${LOCALBASE}/libexec/tftpd:${PORTSDIR}/misc/emulab-tftp-hpa \
	sudo:${PORTSDIR}/security/sudo \
	wget:${PORTSDIR}/ftp/wget \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/XML/Parser:${PORTSDIR}/textproc/p5-XML-Parser \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/RPC/XML.pm:${PORTSDIR}/net/p5-RPC-XML \
	${LOCALBASE}/include/boost:${PORTSDIR}/devel/boost \
	gmake:${PORTSDIR}/devel/gmake \
	kmetis:${PORTSDIR}/math/metis \
        curl:${PORTSDIR}/ftp/curl \
	${PYTHON_SITELIBDIR}/MySQLdb/times.py:${PORTSDIR}/databases/py-MySQLdb \
	mysqlhotcopy:${PORTSDIR}/databases/mysql50-scripts

.if ${PORTVERSION} == "3.0"
# must handle patching of m2crypto when the port is built
RUN_DEPENDS+=	\
	${PYTHON_PKGNAMEPREFIX}m2crypto>0:${PORTSDIR}/misc/emulab-py-m2crypto
.else
# for older version we can patch the standard m2crypto once installed
RUN_DEPENDS+=	\
	${PYTHON_PKGNAMEPREFIX}m2crypto>0:${PORTSDIR}/security/py-m2crypto
.endif

.if (${PORTVERSION} == "2.1" || ${PORTVERSION} == "3.0")
.if ${PORTVERSION} == "3.0"
RUN_DEPENDS+=	\
	dhcpd:${PORTSDIR}/net/isc-dhcp30-server
.else
RUN_DEPENDS+=	\
	dhcpd:${PORTSDIR}/net/isc-dhcp3-server
.endif
RUN_DEPENDS+=	\
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/XML/Parser.pm:${PORTSDIR}/textproc/p5-XML-Parser
# not getting installed via dependencies?
RUN_DEPENDS+=	\
	${X11BASE}/libdata/xorg/libraries:${PORTSDIR}/x11/xorg-libraries \
	${LOCALBASE}/libexec/apache/libphp4.so:${PORTSDIR}/misc/emulab-php4
.endif

.if ${OSMAJOR} >= 6
RUN_DEPENDS+= \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/DBI.pm:${PORTSDIR}/databases/p5-DBI \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/XML/Simple.pm:${PORTSDIR}/textproc/p5-XML-Simple \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/XML/LibXML.pm:${PORTSDIR}/textproc/p5-XML-LibXML \
	swig:${PORTSDIR}/devel/swig13 \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/Date/Parse.pm:${PORTSDIR}/devel/p5-TimeDate
.else
RUN_DEPENDS+= cvsup:${PORTSDIR}/net/cvsup-without-gui \
	${LOCALBASE}/libexec/apache/libphp4.so:${PORTSDIR}/www/mod_php4 \
	${LOCALBASE}/include/php/ext/openssl/php_openssl.h:${PORTSDIR}/lang/php4-extensions \
	${LOCALBASE}/lib/tcl8.4/sql1.0/libTclMySQL.so.1:${PORTSDIR}/databases/tcl-Mysql \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/DBI.pm:${PORTSDIR}/databases/p5-DBI-137 \
	swig1.1:${PORTSDIR}/devel/swig11 \
	${LOCALBASE}/sbin/named:${PORTSDIR}/dns/bind9
.endif

USE_PERL5=	yes
USE_PYTHON=	yes

NO_BUILD=	yes

do-install:	# empty

.include <bsd.port.mk>

vars:
	@echo "PORTVERSION=$(PORTVERSION)"
	@echo ".MAKEFLAGS=$(.MAKEFLAGS)"
	@echo "BUILD_DEPENDS=$(BUILD_DEPENDS)"
	@echo "LIB_DEPENDS=$(LIB_DEPENDS)"
	@echo "RUN_DEPENDS=$(RUN_DEPENDS)"
