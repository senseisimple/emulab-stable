# New ports collection makefile for: elvind
# Date created:         5 February, 2003
# Whom:                 ricci@cs.utah.edu

PORTNAME=	elvind
PORTVERSION=	4.0.3
CATEGORIES=	misc

MAINTAINER=	ricci@cs.utah.edu

RESTRICTED=	"License prohibits redistribution"
NO_PACKAGE=	"Binary redistribution prohibited"
NO_CDROM=	"License prohibits redistribution"

.include <bsd.port.pre.mk>

.if !exists(${DISTDIR}/${DISTNAME}${EXTRACT_SUFX})
IGNORE=         may not be automatically fetched due to licensing\
		restrictions.  Please grab ${DISTFILES} from \
		http://elvin.dstc.com/projects/elvind, and place it in \
		${DISTDIR}.
.endif

HAS_CONFIGURE=	yes
CONFIGURE_ARGS=	--enable-threads --prefix=${PREFIX}

USE_GMAKE=	yes

LIB_DEPENDS=	vin4.2:${PORTSDIR}/misc/libelvin

MAN5=		elvind.conf.5
MAN8=		elvind.8 epf.8

.include <bsd.port.post.mk>
