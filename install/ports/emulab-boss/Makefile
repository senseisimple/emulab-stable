# New ports collection makefile for: emulab-boss
# Date created:         28 January 2003
# Whom:                 ricci@cs.utah.edu

# XXX: Need to get isc-dhcpd2, somehow!

PORTNAME=	emulab-boss
PORTVERSION=	1.0
CATEGORIES=	misc
MASTER_SITES=	#none
DISTFILES=	#none
EXTRACT_ONLY=	#none

MAINTAINER=	ricci@cs.utah.edu

LIB_DEPENDS= gd.4:${PORTSDIR}/graphics/gd2 \
	lthread.2:${PORTSDIR}/devel/linuxthreads \
	mhash.2:${PORTSDIR}/security/mhash \
	mysqlclient.10:${PORTSDIR}/databases/mysql323-client \
	snmp.4:${PORTSDIR}/net/net-snmp4 \
	vin4.2:${PORTSDIR}/misc/libelvin

RUN_DEPENDS=	${LOCALBASE}/etc/apache/ssl.crl:${PORTSDIR}/www/apache13-modssl \
	cvsup:${PORTSDIR}/net/cvsup-without-gui \
	fping:${PORTSDIR}/net/fping \
	neato:${PORTSDIR}/graphics/graphviz \
	${LOCALBASE}/libexec/mysqld:${PORTSDIR}/databases/mysql323-server \
	${LOCALBASE}/libexec/apache/libphp4.so:${PORTSDIR}/www/mod_php4 \
	anytopnm:${PORTSDIR}/graphics/netpbm \
	tclsh8.3:${PORTSDIR}/lang/tcl83 \
	otcl:${PORTSDIR}/lang/otcl \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/GD.pm:${PORTSDIR}/graphics/p5-GD \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/SNMP.pm:${PORTSDIR}/net/p5-SNMP \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/SNMP_Session.pm:${PORTSDIR}/net/p5-SNMP_Session \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/BSD/Resource.pm:${PORTSDIR}/devel/p5-BSD-Resource \
	rpm:${PORTSDIR}/archivers/rpm \
	${LOCALBASE}/lib/tcl8.3/sql1.0/libTclMySQL.so.1:${PORTSDIR}/databases/tcl-Mysql \
	xvcg:${PORTSDIR}/graphics/vcg \
	tftp-hpa:${PORTSDIR}/misc/tftp-hpa \
	elvind:${PORTSDIR}/misc/elvind

BUILD_DEPENDS=	 ${X11BASE}/include/X11/xpm.h:${PORTSDIR}/graphics/xpm \
	${LOCALBASE}/include/boost:${PORTSDIR}/devel/boost \
	gmake:${PORTSDIR}/devel/gmake \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/DBI.pm:${PORTSDIR}/databases/p5-DBI \
	${LOCALBASE}/lib/perl5/site_perl/${PERL_VER}/${PERL_ARCH}/Mysql.pm:${PORTSDIR}/databases/p5-Mysql \
	swig1.1:${PORTSDIR}/devel/SWIG


USE_PERL5=	yes

NO_BUILD=	yes

do-install:	# empty

.include <bsd.port.mk>
