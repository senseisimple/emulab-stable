# New ports collection makefile for: emulab-protogeni
# Date created:         21 July 2009
# Whom:                 stoller@flux.utah.edu

# Need the major version number to figure out to do.
OSMAJOR!= /usr/bin/uname -r | /usr/bin/sed -e 's/\..*//'
OSMINOR!= /usr/bin/uname -r | /usr/bin/sed -e 's/[0-9][0-9]*\.\([0-9][0-9]*\).*/\1/'

PORTNAME=	emulab-protogeni
PORTVERSION=	1.0
.if ${OSMAJOR} >= 7
.if ${OSMINOR} >= 3
PORTVERSION=	1.1
.endif
.endif
CATEGORIES=	misc
MASTER_SITES=	#none
DISTFILES=	#none
EXTRACT_ONLY=	#none
COMMENT=	"Meta-port for a protogeni site"

MAINTAINER=	stoller@cs.utah.edu

# XXX this crap should not be needed!
.if ${PORTVERSION} == "1.1"
PHP_VER=	5
PERL_VER=	5.10.1
.MAKEFLAGS+=	WITH_OPENSSL_BASE=yes
.else
PHP_VER=	4
.MAKEFLAGS+=	WITH_OPENSSL_PORT=yes
.endif

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

LIB_DEPENDS= xml2.5:${PORTSDIR}/textproc/libxml2

RUN_DEPENDS+= \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/Crypt/SSLeay.pm:${PORTSDIR}/security/p5-Crypt-SSLeay \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/XML/LibXML.pm:${PORTSDIR}/textproc/p5-XML-LibXML \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/Frontier/Client.pm:${PORTSDIR}/net/p5-Frontier-RPC \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/Crypt/X509.pm:${PORTSDIR}/security/p5-Crypt-X509 \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/Crypt/OpenSSL/X509.pm:${PORTSDIR}/security/p5-Crypt-OpenSSL-X509 \
${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/XML/SemanticDiff.pm:${PORTSDIR}/textproc/p5-XML-SemanticDiff \
	xmlsec1:${PORTSDIR}/security/xmlsec1

USE_PERL5=	yes
USE_PYTHON=	yes

NO_BUILD=	yes

do-install:	# empty

.include <bsd.port.mk>
