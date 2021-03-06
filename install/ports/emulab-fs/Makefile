# New ports collection makefile for: emulab-fs
#

# Need the major version number to figure out to do.
OSMAJOR!= /usr/bin/uname -r | /usr/bin/sed -e 's/\..*//'
OSMINOR!= /usr/bin/uname -r | /usr/bin/sed -e 's/[0-9][0-9]*\.\([0-9][0-9]*\).*/\1/'

PORTNAME=	emulab-fs
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
COMMENT=	"Meta-port for an Emulab fs node"

MAINTAINER=	testbed-ops@flux.utah.edu

#
# Select PHP and Apache versions
# Fix up some port diffs
#
.if ${PORTVERSION} == "3.1"
PHP_VER=	5
.else
PHP_VER=	4
.endif

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
# For samba, so that it doesn't suck in everything under the sun
#
.MAKEFLAGS+=	-DWITHOUT_WINBIND -DWITHOUT_LDAP -DWITHOUT_POPT -DWITHOUT_CUPS

#
# This prevents any ports from trying to run interactively - some, ask silly
# questions without this option.
#
.MAKEFLAGS+=	-DBATCH

RUN_DEPENDS= gmake:${PORTSDIR}/devel/gmake \
	sudo:${PORTSDIR}/security/sudo \
	rsync:${PORTSDIR}/net/rsync \
	smbd:${PORTSDIR}/net/samba3

NO_BUILD=	yes

do-install:	# empty

.include <bsd.port.mk>
