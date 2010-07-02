PORTNAME=	RPC-Async
PORTVERSION=	1.05
CATEGORIES=	misc perl5
MASTER_SITES=	${MASTER_SITE_PERL_CPAN}
MASTER_SITE_SUBDIR=	IO/TLBDK
PKGNAMEPREFIX=	p5-

MAINTAINER=	sklower@berkeley.edu
COMMENT=	Perl5 module providing a framework for Asynchronous RPC

RUN_DEPENDS=    ${SITE_PERL}/IO/EventMux.pm:${PORTSDIR}/misc/p5-IO-EventMux \
		${SITE_PERL}/Privileges/Drop.pm:${PORTSDIR}/misc/p5-Privileges-Drop

BUILD_DEPENDS=  ${RUN_DEPENDS}


PERL_CONFIGURE=	5.8.0+
PERL_MODBUILD=	yes

MAN3=		RPC::Async.3 \
		RPC::Async::Client.3 \
		RPC::Async::Coderef.3 \
		RPC::Async::Server.3 \
		RPC::Async::URL.3 \
		RPC::Async::Util.3

.include <bsd.port.mk>
