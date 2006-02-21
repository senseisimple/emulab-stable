# New ports collection makefile for: emulab-fs
#

# Need the major version number to figure out to do.
OSMAJOR!= /usr/bin/uname -r | /usr/bin/sed -e 's/\..*//'

PORTNAME=	emulab-fs
.if ${OSMAJOR} >= 5
PORTVERSION=	2.0
.else
PORTVERSION=	1.4
.endif
CATEGORIES=	misc
MASTER_SITES=	#none
DISTFILES=	#none
EXTRACT_ONLY=	#none
COMMENT=	"Meta-port for an emulab fs node"

MAINTAINER=	hibler@cs.utah.edu

#
# For samba, so that it doesn't suck in everything under the sun
#
.MAKEFLAGS+=	-DWITHOUT_WINBIND -DWITHOUT_LDAP -DWITHOUT_POPT -DWITHOUT_CUPS

#
# We must make sure PERL is built with this.
#
.MAKEFLAGS+=	ENABLE_SUIDPERL=yes

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
