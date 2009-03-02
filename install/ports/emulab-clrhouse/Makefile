# New ports collection makefile for: emulab-clrhouse
# Date created:         28 February 2009
# Whom:                 stoller@flux.utah.edu

# Need the major version number to figure out to do.
OSMAJOR!= /usr/bin/uname -r | /usr/bin/sed -e 's/\..*//'
OSMINOR!= /usr/bin/uname -r | /usr/bin/sed -e 's/[0-9][0-9]*\.\([0-9][0-9]*\).*/\1/'

PORTNAME=	emulab-clrhouse
.if ${OSMAJOR} >= 6
.if ${OSMINOR} >= 3
PORTVERSION=	1.1
.else
PORTVERSION=	1.0
.endif
.endif
CATEGORIES=	misc
MASTER_SITES=	#none
DISTFILES=	#none
EXTRACT_ONLY=	#none
COMMENT=	"Meta-port for a protogeni clearinghouse"

MAINTAINER=	stoller@cs.utah.edu

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
# Have not taken the plunge to PHP5
#
.MAKEFLAGS+=	PHP_VER=4

#
# Various ports need to agree on what version of apache. Make sure this
# agrees with what is in the emulab-ops and emulab-php4 metaports.
#
.MAKEFLAGS+=	APACHE_PORT=www/apache13-modssl WITH_OPENSSL_PORT=yes

#
# This prevents any ports from trying to run interactively - some, like the
# net-snmp port, ask silly questions without this option.
#
.MAKEFLAGS+=	-DBATCH

BUILD_DEPENDS=	\
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/Mysql.pm:${PORTSDIR}/databases/p5-DBD-mysql50 \
	${LOCALBASE}/libexec/apache/libphp4.so:${PORTSDIR}/misc/emulab-php4 \

LIB_DEPENDS= mhash.2:${PORTSDIR}/security/mhash \
	mysqlclient.15:${PORTSDIR}/databases/mysql50-client \
	pubsub.1:${PORTSDIR}/misc/emulab-pubsub

RUN_DEPENDS=	\
	${LOCALBASE}/etc/apache/ssl.crl:${PORTSDIR}/www/apache13-modssl \
	${LOCALBASE}/libexec/apache/libauth_mysql.so:${PORTSDIR}/www/mod_auth_mysql \
	${LOCALBASE}/libexec/apache/libphp4.so:${PORTSDIR}/misc/emulab-php4 \
	rsync:${PORTSDIR}/net/rsync \
	${LOCALBASE}/libexec/mysqld:${PORTSDIR}/databases/mysql50-server \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/BSD/Resource.pm:${PORTSDIR}/devel/p5-BSD-Resource \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/Time/HiRes.pm:${PORTSDIR}/devel/p5-Time-HiRes \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/MD5.pm:${PORTSDIR}/security/p5-MD5 \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/IO/Pty.pm:${PORTSDIR}/devel/p5-IO-Tty \
	sudo:${PORTSDIR}/security/sudo \
	wget:${PORTSDIR}/ftp/wget \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/XML/Parser:${PORTSDIR}/textproc/p5-XML-Parser \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/RPC/XML.pm:${PORTSDIR}/net/p5-RPC-XML \
	${PYTHON_SITELIBDIR}/M2Crypto/ASN1.py:${PORTSDIR}/security/py-m2crypto \
	gmake:${PORTSDIR}/devel/gmake \
	${PYTHON_SITELIBDIR}/MySQLdb/times.py:${PORTSDIR}/databases/py-MySQLdb \
	mysqlhotcopy:${PORTSDIR}/databases/mysql50-scripts

RUN_DEPENDS+= \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/DBI.pm:${PORTSDIR}/databases/p5-DBI \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/XML/Simple.pm:${PORTSDIR}/textproc/p5-XML-Simple \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/Date/Parse.pm:${PORTSDIR}/devel/p5-TimeDate

RUN_DEPENDS+= \
	${LOCALBASE}/lib/libxml2.so:${PORTSDIR}/textproc/libxml2 \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/Frontier/Client.pm:${PORTSDIR}/net/p5-Frontier-RPC \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}${PERL_ARCH}/XML/LibXML.pm:${PORTSDIR}/textproc/p5-XML-LibXML \
	xmlsec1:${PORTSDIR}/security/xmlsec1 \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}${PERL_ARCH}/Crypt/SSLeay.pm:${PORTSDIR}/security/p5-Crypt-SSLeay

USE_PERL5=	yes
USE_PYTHON=	yes

NO_BUILD=	yes

do-install:	# empty

.include <bsd.port.mk>
