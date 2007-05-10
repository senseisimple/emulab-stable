# ports collection makefile for: emulab-pubsub-elvincompat

PORTNAME=	pubsub-elvincompat
PORTVERSION=	0.8
CATEGORIES=	misc
MASTER_SITES=	http://www.emulab.net/distributions/

MAINTAINER=	stoller@flux.utah.edu
COMMENT=	"Simple publish subscribe system for Emulab, Elvind compatible"

MAKEFILE=	GNUmakefile
USE_GMAKE=	yes
INSTALLS_SHLIB=	yes
NO_CHECKSUM=    yes

CONFLICTS=	pubsub-[0-9]*
MAKE_FLAGS=	ELVIN_COMPAT=yes

.include <bsd.port.mk>
