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

CONFLICTS=	pubsub-[0-9]*
MAKE_FLAGS=	ELVIN_COMPAT=yes

LIB_DEPENDS=	vin4.2:${PORTSDIR}/misc/libelvin
RUN_DEPENDS=	${LOCALBASE}/libexec/elvind:${PORTSDIR}/misc/elvind

.include <bsd.port.mk>
