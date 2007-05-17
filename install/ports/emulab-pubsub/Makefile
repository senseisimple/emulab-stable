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

CONFLICTS=	pubsub-elvincompat-[0-9]*
MAKE_FLAGS=	ELVIN_COMPAT=

.include <bsd.port.mk>
