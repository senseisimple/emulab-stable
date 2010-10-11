# New ports collection makefile for: emulab-php5
# Date created:         09 February 2006
# Whom:                 stoller@flux.utah.edu

#
# These are for the php5-extensions port
#
WITH_XMLRPC=yes
WITH_CURL=yes
WITH_OPENSSL=yes
WITH_BZ2=yes
WITH_GD=yes
WITH_MCRYPT=yes
WITH_MHASH=yes
WITH_MYSQL=yes
WITH_XML=yes
# Additions for mediawiki
#WITH_ICONV=yes
#WITH_READLINE=yes
#WITH_MBSTRING=yes

WITHOUT_SQLITE=true
WITHOUT_PDO_SQLITE=true

#
# For the PHP ports, so they will depend on the actual version of apache we're
# installing
#
APACHE_PORT=www/apache13-modssl

#
# This prevents the option screen.
#
BATCH=yes

#
# But need this too!
#
.MAKEFLAGS+=	-DBATCH APACHE_PORT=www/apache13-modssl WITH_APACHE=yes

.include "../../lang/php5-extensions/Makefile"
