# New ports collection makefile for: emulab-ops
# Date created:         28 January 2003
# Whom:                 ricci@cs.utah.edu

# Need the major version number to figure out to do.
OSMAJOR!= /usr/bin/uname -r | /usr/bin/sed -e 's/\..*//'
OSMINOR!= /usr/bin/uname -r | /usr/bin/sed -e 's/[0-9][0-9]*\.\([0-9][0-9]*\).*/\1/'

PORTNAME=	emulab-ops
.if ${OSMAJOR} > 6
.if ${OSMINOR} > 2
PORTVERSION=	3.1
.else
PORTVERSION=	3.0
.endif
.else
.if ${OSMAJOR} > 5
.if ${OSMINOR} >= 3
PORTVERSION=	2.1
.else
PORTVERSION=	2.0
.endif
.else
PORTVERSION=	1.4
.endif
.endif
CATEGORIES=	misc
MASTER_SITES=	#none
DISTFILES=	#none
EXTRACT_ONLY=	#none
COMMENT=	"Meta-port for an Emulab ops node"

MAINTAINER=	testbed-ops@flux.utah.edu

#
# Select PHP and Apache versions
# Fix up some port diffs
#
.if ${PORTVERSION} == "3.1"
PHP_VER=	5
PERL_VER=	5.10.1
.else
PHP_VER=	4
.endif

#
# We set this so that the mysql server port doesn't do name resolution
#
.MAKEFLAGS+=	-DSKIP_DNS_CHECK

#
# We must make sure PERL is built with this.
#
.MAKEFLAGS+=	ENABLE_SUIDPERL=yes
.if ${PORTVERSION} == "3.1"
.MAKEFLAGS+=	PERL_VERSION=5.10.1 PERL_VER=5.10.1 PERL_ARCH=mach
.else
.if ${PORTVERSION} == "3.0"
.MAKEFLAGS+=	PERL_VERSION=5.10.0 PERL_VER=5.10.0 PERL_ARCH=mach
.endif
.endif

#
# For SWIG, so that it doesn't feel the need to suck in php and apache
# XXX Something is broken on FreeBSD 6.X and SWIG_LANGUAGES='x y z' breaks.
#
.MAKEFLAGS+=	SWIG_LANGUAGES= 
.MAKEFLAGS+=    WANT_SWIG_PERL=yes WANT_SWIG_PYTHON=yes WANT_SWIG_TCL=yes

#
# For PHP, specify the version and that we want the Apache module
# Version is specified via PHP_VER above.
#
.MAKEFLAGS+=	WANT_PHP_MOD=yes

#
# Various ports need to agree on what version of apache. Make sure this
# agrees with what is in the emulab-ops and emulab-php* metaports.
#
.MAKEFLAGS+=	-DWITH_APACHE
.MAKEFLAGS+=	APACHE_PORT=www/apache13-modssl

#
# For samba, so that it doesn't suck in everything under the sun
#
.MAKEFLAGS+=	-DWITHOUT_WINBIND -DWITHOUT_LDAP -DWITHOUT_POPT -DWITHOUT_CUPS

#
# This prevents any ports from trying to run interactively - some, ask silly
# questions without this option.
#
.MAKEFLAGS+=	-DBATCH

#
# XXX not rbuild dependencies, but these Emulab-specific ports need to get
# installed before any attempt to install the standard versions.
#
.if (${PORTVERSION} == "2.1" || ${PORTVERSION} == "3.0" || ${PORTVERSION} == "3.1")
BUILD_DEPENDS=	\
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/Mysql.pm:${PORTSDIR}/misc/p5-DBD-mysql50-old \
	${LOCALBASE}/libexec/apache/libphp${PHP_VER}.so:${PORTSDIR}/misc/emulab-php${PHP_VER}
.else
BUILD_DEPENDS=	\
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/Mysql.pm:${PORTSDIR}/databases/p5-DBD-mysql50
.endif

LIB_DEPENDS= lthread.[236]:${PORTSDIR}/devel/linuxthreads \
	ulsshxmlrpcpp.0:${PORTSDIR}/misc/ulsshxmlrpcpp

.if defined(ELVIN_COMPAT)
LIB_DEPENDS+= vin4.2:${PORTSDIR}/misc/libelvin \
	pubsub.1:${PORTSDIR}/misc/emulab-pubsub-elvincompat
.else
LIB_DEPENDS+= pubsub.1:${PORTSDIR}/misc/emulab-pubsub
.endif

.if ${OSMAJOR} >= 6
LIB_DEPENDS+= mhash.2:${PORTSDIR}/security/mhash \
	mysqlclient.15:${PORTSDIR}/databases/mysql50-client
.endif

RUN_DEPENDS= gmake:${PORTSDIR}/devel/gmake \
	sudo:${PORTSDIR}/security/sudo \
	rsync:${PORTSDIR}/net/rsync \
	otclsh:${PORTSDIR}/lang/otcl \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/BSD/Resource.pm:${PORTSDIR}/devel/p5-BSD-Resource \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/Time/HiRes.pm:${PORTSDIR}/devel/p5-Time-HiRes \
	wget:${PORTSDIR}/ftp/wget

.if defined(ELVIN_COMPAT)
RUN_DEPENDS+= ${LOCALBASE}/libexec/elvind:${PORTSDIR}/misc/elvind
.endif

.if (${PORTVERSION} == "3.0" || ${PORTVERSION} == "3.1")
# must handle patching of m2crypto when the port is built
# also needs swig to install!
RUN_DEPENDS+=	\
	${PYTHON_PKGNAMEPREFIX}m2crypto>0:${PORTSDIR}/misc/emulab-py-m2crypto \
	swig:${PORTSDIR}/devel/swig13
.else
# for older version we can patch the standard m2crypto once installed
RUN_DEPENDS+=	\
	${PYTHON_PKGNAMEPREFIX}m2crypto>0:${PORTSDIR}/security/py-m2crypto
.endif

.if ${OSMAJOR} >= 6
RUN_DEPENDS+= \
	${LOCALBASE}/etc/apache/ssl.crl:${PORTSDIR}/www/apache13-modssl \
	mod_auth_mysql>1:${PORTSDIR}/www/mod_auth_mysql \
	${LOCALBASE}/libexec/mysqld:${PORTSDIR}/databases/mysql50-server \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/DBI.pm:${PORTSDIR}/databases/p5-DBI \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/XML/Parser:${PORTSDIR}/textproc/p5-XML-Parser \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/XML/Simple.pm:${PORTSDIR}/textproc/p5-XML-Simple \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/CGI/Session.pm:${PORTSDIR}/www/p5-CGI-Session \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/GD/Graph.pm:${PORTSDIR}/graphics/p5-GD-Graph \
	${LOCALBASE}/sbin/cvsd:${PORTSDIR}/devel/cvsd \
        ${LOCALBASE}/share/adodb/adodb.inc.php:${PORTSDIR}/databases/adodb \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/HTML/Parser.pm:${PORTSDIR}/www/p5-HTML-Parser \
	${PYTHON_SITELIBDIR}/MySQLdb/times.py:${PORTSDIR}/databases/py-MySQLdb \
        curl:${PORTSDIR}/ftp/curl \
        snort:${PORTSDIR}/security/snort \
	mysqlhotcopy:${PORTSDIR}/databases/mysql50-scripts
.else
RUN_DEPENDS+= smbd:${PORTSDIR}/net/samba3
.endif

.if (${PORTVERSION} == "2.1" || ${PORTVERSION} == "3.0" || ${PORTVERSION} == "3.1")
# not sure why this is just needed for 6.3
RUN_DEPENDS+= ${X11BASE}/libdata/xorg/libraries:${PORTSDIR}/x11/xorg-libraries
.endif

NO_BUILD=	yes
USE_PYTHON=	yes
USE_PERL5=	yes

do-install:	# empty

.include <bsd.port.mk>
