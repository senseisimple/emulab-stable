# New ports collection makefile for: emulab-ops
# Date created:         28 January 2003
# Whom:                 ricci@cs.utah.edu

PORTNAME=	emulab-ops
PORTVERSION=	1.4
CATEGORIES=	misc
MASTER_SITES=	#none
DISTFILES=	#none
EXTRACT_ONLY=	#none
COMMENT=	"Meta-port for an emulab ops node"

MAINTAINER=	ricci@cs.utah.edu

#
# For SWIG, so that it doesn't feel the need to suck in php and apache
#
.MAKEFLAGS+=	SWIG_LANGUAGES="perl python tcl"

LIB_DEPENDS= lthread.2:${PORTSDIR}/devel/linuxthreads \
	vin4.2:${PORTSDIR}/misc/libelvin \
	ulsshxmlrpcpp.0:${PORTSDIR}/misc/ulsshxmlrpcpp

RUN_DEPENDS= gmake:${PORTSDIR}/devel/gmake \
	sudo:${PORTSDIR}/security/sudo \
	otclsh:${PORTSDIR}/lang/otcl \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/BSD/Resource.pm:${PORTSDIR}/devel/p5-BSD-Resource \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/Time/HiRes.pm:${PORTSDIR}/devel/p5-Time-HiRes \
	wget:${PORTSDIR}/ftp/wget \
	${LOCALBASE}/libexec/elvind:${PORTSDIR}/misc/elvind \
	${PYTHON_SITELIBDIR}/M2Crypto/ASN1.py:${PORTSDIR}/security/py-m2crypto


NO_BUILD=	yes
USE_PYTHON=	yes

do-install:	# empty

.include <bsd.port.mk>
