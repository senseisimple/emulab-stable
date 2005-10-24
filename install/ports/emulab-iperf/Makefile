# New ports collection makefile for:	iperf
# Date Created:				20 May 2001
# Whom:					Pete Fritchman <petef@databits.net>
#

PORTNAME=	iperf
PORTVERSION=	2.0.2
CATEGORIES=	misc
MASTER_SITES=	http://dast.nlanr.net/Projects/Iperf2.0/ \
		${MASTER_SITE_SOURCEFORGE}
MASTER_SITE_SUBDIR=	${PORTNAME}
PKGNAMEPREFIX=	emulab-

MAINTAINER=	testbed-ops@flux.utah.edu
COMMENT=	"The iperf distribution as configured for Emulab"

NO_CDROM=	"can't charge a fee for the software"

HAS_CONFIGURE=	yes

PLIST_FILES=	bin/emulab-iperf

.include <bsd.port.pre.mk>

CONFIGURE_ARGS+=	--prefix=${PREFIX}
CONFIGURE_ARGS+=	--disable-ipv6

.include <bsd.port.post.mk>
