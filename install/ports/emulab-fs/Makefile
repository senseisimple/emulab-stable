# New ports collection makefile for: emulab-ops
# Date created:         28 January 2003
# Whom:                 ricci@cs.utah.edu

PORTNAME=	emulab-fs
PORTVERSION=	1.4
CATEGORIES=	misc
MASTER_SITES=	#none
DISTFILES=	#none
EXTRACT_ONLY=	#none
COMMENT=	"Meta-port for an emulab fs node"

MAINTAINER=	hibler@cs.utah.edu

#
# For samba, so that it doesn't suck in everything under the sun
#
.MAKEFLAGS+=	-DSAMBA_SUBPORT -DWITHOUT_WINBIND -DWITHOUT_LDAP -DWITHOUT_POPT

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
