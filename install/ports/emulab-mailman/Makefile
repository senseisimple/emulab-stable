# New ports collection makefile for: emulab-mailman
# Date created:         23 February 2006
# Whom:                 stoller@flux.utah.edu

#
# We happen to run the web server as "nobody"
#
CGI_GID=	nobody

#
# This prevents the option screen.
#
BATCH=yes

#
# But need this too!
#
.MAKEFLAGS+=	-DBATCH CGI_GID=nobody

MASTERDIR=	${.CURDIR}/../../mail/mailman

.include "${MASTERDIR}/Makefile"
