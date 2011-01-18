# ports collection makefile for: emulab-pubsub

PORTNAME=	pubsub
PORTVERSION=	0.95
CATEGORIES=	misc
MASTER_SITES=	http://www.emulab.net/distributions/
DISTFILES=      ${PORTNAME}-${PORTVERSION}.tar.gz
WRKSRC=         ${WRKDIR}/${PORTNAME}-${PORTVERSION}

MAINTAINER=	testbed-ops@flux.utah.edu
COMMENT=	"Simple publish subscribe system for Emulab"

MAKEFILE=	GNUmakefile
USE_GMAKE=	yes
INSTALLS_SHLIB=	yes

CONFLICTS=	pubsub-elvincompat-[0-9]*
MAKE_FLAGS=	ELVIN_COMPAT= ELVIN_NOELVIN=1

.include <bsd.port.mk>
