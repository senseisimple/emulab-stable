# New ports collection makefile for: emulab-ops
# Date created:         28 January 2003
# Whom:                 ricci@cs.utah.edu

PORTNAME=	emulab-ops
PORTVERSION=	1.2
CATEGORIES=	misc
MASTER_SITES=	#none
DISTFILES=	#none
EXTRACT_ONLY=	#none

MAINTAINER=	ricci@cs.utah.edu

LIB_DEPENDS= lthread.2:${PORTSDIR}/devel/linuxthreads \
	vin4.2:${PORTSDIR}/misc/libelvin \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/BSD/Resource.pm:${PORTSDIR}/devel/p5-BSD-Resource

RUN_DEPENDS= gmake:${PORTSDIR}/devel/gmake \
	sudo:${PORTSDIR}/security/sudo \
	otcl:${PORTSDIR}/lang/otcl

NO_BUILD=	yes

do-install:	# empty

.include <bsd.port.mk>
