# ports collection makefile for: emulab-pubsub

PORTNAME=	pubsub
PORTVERSION=	0.8
CATEGORIES=	misc
MASTER_SITES=	http://www.emulab.net/distributions/

MAINTAINER=	stoller@flux.utah.edu
COMMENT=	"Simple publish subscribe system for Emulab"

MAKEFILE=	GNUmakefile
USE_GMAKE=	yes
INSTALLS_SHLIB=	yes
NO_CHECKSUM=    yes

.if exists(/usr/local/libexec/elvind)
PLIST_FILES=    etc/rc.d/3.elvin_gateway libexec/elvin_gateway
MAKE_ARGS=	ELVIN_COMPAT=1
.endif

.include <bsd.port.mk>
