
YUM = yum

YUM_INSTALL =

# Things we actually want
#
YUM_INSTALL += arpwatch
YUM_INSTALL += bash-doc
YUM_INSTALL += cproto
YUM_INSTALL += ctags
YUM_INSTALL += enscript
YUM_INSTALL += expect
YUM_INSTALL += glib2-devel
YUM_INSTALL += gtk+-devel
YUM_INSTALL += hexedit
YUM_INSTALL += lm_sensors
YUM_INSTALL += lm_sensors-devel
YUM_INSTALL += lynx
YUM_INSTALL += ncftp
YUM_INSTALL += pcre-devel
YUM_INSTALL += pine
YUM_INSTALL += psacct
YUM_INSTALL += tftp
YUM_INSTALL += zsh

# Things that they depend on:
#
YUM_INSTALL += glib-devel
YUM_INSTALL += perl-CGI

# no needed?
# YUM_INSTALL += nasm

# no such
# bdflush
# console-tools
# gated
# pwdb
# python-xmlrpc

###############################################################################

all:
	echo foo

install:
	$(YUM) -y install $(YUM_INSTALL)

###############################################################################

## End of file.
