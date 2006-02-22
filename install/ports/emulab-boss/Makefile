# New ports collection makefile for: emulab-boss
# Date created:         28 January 2003
# Whom:                 ricci@cs.utah.edu

# Need the major version number to figure out to do.
OSMAJOR!= /usr/bin/uname -r | /usr/bin/sed -e 's/\..*//'

PORTNAME=	emulab-boss
.if ${OSMAJOR} >= 5
PORTVERSION=	2.0
.else
PORTVERSION=	1.7
.endif
CATEGORIES=	misc
MASTER_SITES=	#none
DISTFILES=	#none
EXTRACT_ONLY=	#none
COMMENT=	"Meta-port for an emulab boss node"

MAINTAINER=	ricci@cs.utah.edu

#
# We set this so that the mysql server port doesn't do name resolution
#
.MAKEFLAGS+=	-DSKIP_DNS_CHECK

#
# We must make sure PERL is built with this.
#
.MAKEFLAGS+=	ENABLE_SUIDPERL=yes

#
# For SWIG, so that it doesn't feel the need to suck in php and apache
# XXX Something is broken on FreeBSD 6.X and SWIG_LANGUAGES='x y z' breaks.
#
.MAKEFLAGS+=	SWIG_LANGUAGES= 
.MAKEFLAGS+=    WANT_SWIG_PERL=yes WANT_SWIG_PYTHON=yes WANT_SWIG_TCL=yes

#
# Various ports need to agree on what version of apache. Make sure this
# agrees with what is in the emulab-ops and emulab-php4 metaports.
#
.MAKEFLAGS+=	APACHE_PORT=www/apache13-modssl

#
# This prevents any ports from trying to run interactively - some, like the
# net-snmp port, ask silly questions without this option.
#
.MAKEFLAGS+=	-DBATCH

LIB_DEPENDS= gd.4:${PORTSDIR}/graphics/gd \
	lthread.[23]:${PORTSDIR}/devel/linuxthreads \
	mhash.2:${PORTSDIR}/security/mhash \
	mysqlclient.10:${PORTSDIR}/databases/mysql323-client \
	snmp.4:${PORTSDIR}/net-mgmt/net-snmp4 \
	vin4.2:${PORTSDIR}/misc/libelvin \
	ulsshxmlrpcpp.0:${PORTSDIR}/misc/ulsshxmlrpcpp

RUN_DEPENDS=	\
	${LOCALBASE}/etc/apache/ssl.crl:${PORTSDIR}/www/apache13-modssl \
	${LOCALBASE}/libexec/apache/libauth_mysql.so:${PORTSDIR}/www/mod_auth_mysql \
	fping:${PORTSDIR}/net/fping \
	rsync:${PORTSDIR}/net/rsync \
	neato:${PORTSDIR}/graphics/graphviz \
        swish-e:${PORTSDIR}/www/swish-e \
	${LOCALBASE}/libexec/mysqld:${PORTSDIR}/databases/mysql323-server \
	anytopnm:${PORTSDIR}/graphics/netpbm \
	tclsh8.3:${PORTSDIR}/lang/tcl83 \
	otclsh:${PORTSDIR}/lang/otcl \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/GD.pm:${PORTSDIR}/misc/p5-GD-old \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/SNMP.pm:${PORTSDIR}/net-mgmt/p5-SNMP \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/SNMP_Session.pm:${PORTSDIR}/net-mgmt/p5-SNMP_Session \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/BSD/Resource.pm:${PORTSDIR}/devel/p5-BSD-Resource \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/Time/HiRes.pm:${PORTSDIR}/devel/p5-Time-HiRes \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/MD5.pm:${PORTSDIR}/security/p5-MD5 \
	rpm:${PORTSDIR}/archivers/rpm \
	xvcg:${PORTSDIR}/graphics/vcg \
	${LOCALBASE}/libexec/tftpd:${PORTSDIR}/misc/emulab-tftp-hpa \
	${LOCALBASE}/libexec/elvind:${PORTSDIR}/misc/elvind \
	dhcpd:${PORTSDIR}/net/isc-dhcp3-server \
	sudo:${PORTSDIR}/security/sudo \
	${LOCALBASE}/sbin/named:${PORTSDIR}/dns/bind9 \
	wget:${PORTSDIR}/ftp/wget \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/XML/Parser:${PORTSDIR}/textproc/p5-XML-Parser \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/RPC/XML.pm:${PORTSDIR}/net/p5-RPC-XML \
	${PYTHON_SITELIBDIR}/M2Crypto/ASN1.py:${PORTSDIR}/security/py-m2crypto \
	${LOCALBASE}/include/boost:${PORTSDIR}/devel/boost \
	gmake:${PORTSDIR}/devel/gmake \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/Mysql.pm:${PORTSDIR}/databases/p5-Mysql \
	kmetis:${PORTSDIR}/math/metis \
        curl:${PORTSDIR}/ftp/curl \
	${PYTHON_SITELIBDIR}/MySQLdb/times.py:${PORTSDIR}/databases/py-MySQLdb

.if ${OSMAJOR} >= 5
RUN_DEPENDS+= \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/DBI.pm:${PORTSDIR}/databases/p5-DBI \
	swig:${PORTSDIR}/devel/swig13
.else
RUN_DEPENDS+= cvsup:${PORTSDIR}/net/cvsup-without-gui \
	${LOCALBASE}/libexec/apache/libphp4.so:${PORTSDIR}/www/mod_php4 \
	${LOCALBASE}/include/php/ext/openssl/php_openssl.h:${PORTSDIR}/lang/php4-extensions \
	${LOCALBASE}/lib/tcl8.4/sql1.0/libTclMySQL.so.1:${PORTSDIR}/databases/tcl-Mysql \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/DBI.pm:${PORTSDIR}/databases/p5-DBI-137 \
	swig1.1:${PORTSDIR}/devel/swig11
.endif

USE_PERL5=	yes
USE_PYTHON=	yes

NO_BUILD=	yes

do-install:	# empty

.include <bsd.port.mk>
