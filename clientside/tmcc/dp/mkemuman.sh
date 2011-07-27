#!/bin/sh
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
. /etc/emulab/paths.sh

ostype=`uname -s`

if [ "$ostype" = "FreeBSD" ];  then
	USERADD="pw useradd"
elif [ "$ostype" = "Linux" ];  then
	USERADD="useradd"
else
	echo "Unsupported OS: $ostype"
	exit 1
fi

$USERADD emulabman -u 65520 -g bin -m -s /bin/tcsh -c "Emulab Man"

if [ ! -d ~emulabman ]; then
	mkdir ~emulabman
	chown emulabman ~emulabman
	chgrp bin ~emulabman
fi

if [ -f $ETCDIR/emulabkey -a ! -d ~emulabman/.ssh ]; then
	cd ~emulabman
	chmod 755 .
	mkdir .ssh
	chown emulabman .ssh
	chgrp bin .ssh
	chmod 700 .ssh
	cd .ssh
	cp $ETCDIR/emulabkey authorized_keys
	chown emulabman authorized_keys
	chgrp bin authorized_keys
	chmod 644 authorized_keys
fi

exit 0
