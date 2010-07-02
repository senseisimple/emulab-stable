# New ports collection makefile for: emulab-php4
# Date created:         09 February 2006
# Whom:                 stoller@flux.utah.edu

#
# These are for the php4-extensions port
#
WITH_MHASH=yes
WITH_XMLRPC=yes
WITH_CURL=yes
WITH_OPENSSL=yes
WITH_BZ2=yes
WITH_GD=yes
WITH_MCRYPT=yes
WITH_MYSQL=yes
WITH_XML=yes
WITH_APACHE=yes
# Additions for mediawiki
#WITH_ICONV=yes
#WITH_IMAGICK=yes
#WITH_READLINE=yes
#WITH_MBSTRING=yes

#
# For the PHP ports, so they will depend on the actual version of apache we're
# installing
#
APACHE_PORT=www/apache13-modssl

#
# This prevents the option screen.
#
BATCH=yes

# Disgusting. When we build the php4 port, we have to tell it we're going
# to be building an SSL extension later. The port appears to be fixed at
# 4.4.2, so this can go when we upgrade our base to that (currently at
# 4.4.1).
#
# And, with 4.4.7 we need to add WITH_APACHE as well
#
.MAKEFLAGS+=    EXTRA_CFLAGS=-DHAVE_OPENSSL_EXT WITH_OPENSSL=yes WITH_APACHE=yes

#
# But need this too!
#
.MAKEFLAGS+=	-DBATCH APACHE_PORT=www/apache13-modssl

.include "../../lang/php4-extensions/Makefile"
