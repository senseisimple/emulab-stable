# New ports collection makefile for: emulab-ops
# Date created:         28 January 2003
# Whom:                 ricci@cs.utah.edu

# Need the major version number to figure out to do.
OSMAJOR!= /usr/bin/uname -r | /usr/bin/sed -e 's/\..*//'

PORTNAME=	emulab-ops
.if ${OSMAJOR} >= 5
PORTVERSION=	2.0
.else
PORTVERSION=	1.4
.endif
CATEGORIES=	misc
MASTER_SITES=	#none
DISTFILES=	#none
EXTRACT_ONLY=	#none
COMMENT=	"Meta-port for an emulab ops node"

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
# For samba, so that it doesn't suck in everything under the sun
#
.MAKEFLAGS+=	-DWITHOUT_WINBIND -DWITHOUT_LDAP -DWITHOUT_POPT -DWITHOUT_CUPS

#
# This prevents any ports from trying to run interactively - some, ask silly
# questions without this option.
#
.MAKEFLAGS+=	-DBATCH

LIB_DEPENDS= lthread.[23]:${PORTSDIR}/devel/linuxthreads \
	vin4.2:${PORTSDIR}/misc/libelvin \
	ulsshxmlrpcpp.0:${PORTSDIR}/misc/ulsshxmlrpcpp

.if ${OSMAJOR} >= 5
LIB_DEPENDS+= mhash.2:${PORTSDIR}/security/mhash \
	mysqlclient.10:${PORTSDIR}/databases/mysql323-client
.endif

RUN_DEPENDS= gmake:${PORTSDIR}/devel/gmake \
	sudo:${PORTSDIR}/security/sudo \
	rsync:${PORTSDIR}/net/rsync \
	otclsh:${PORTSDIR}/lang/otcl \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/BSD/Resource.pm:${PORTSDIR}/devel/p5-BSD-Resource \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/Time/HiRes.pm:${PORTSDIR}/devel/p5-Time-HiRes \
	wget:${PORTSDIR}/ftp/wget \
	${LOCALBASE}/libexec/elvind:${PORTSDIR}/misc/elvind \
	${PYTHON_SITELIBDIR}/M2Crypto/ASN1.py:${PORTSDIR}/security/py-m2crypto

.if ${OSMAJOR} >= 5
RUN_DEPENDS+= \
	${LOCALBASE}/etc/apache/ssl.crl:${PORTSDIR}/www/apache13-modssl \
	${LOCALBASE}/libexec/apache/libauth_mysql.so:${PORTSDIR}/www/mod_auth_mysql \
	${LOCALBASE}/libexec/mysqld:${PORTSDIR}/databases/mysql323-server \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/DBI.pm:${PORTSDIR}/databases/p5-DBI \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/Mysql.pm:${PORTSDIR}/databases/p5-Mysql \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/XML/Parser:${PORTSDIR}/textproc/p5-XML-Parser \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/XML/Simple.pm:${PORTSDIR}/textproc/p5-XML-Simple \
        curl:${PORTSDIR}/ftp/curl
.else
RUN_DEPENDS+= smbd:${PORTSDIR}/net/samba3
.endif

.if ${OSMAJOR} >= 6
RUN_DEPENDS+= ${LOCALBASE}/sbin/cvsd:${PORTSDIR}/devel/cvsd \
              ${LOCALBASE}/share/adodb/adodb.inc.php:${PORTSDIR}/databases/adodb
.endif

NO_BUILD=	yes
USE_PYTHON=	yes
USE_PERL5=	yes

do-install:	# empty

.include <bsd.port.mk>

