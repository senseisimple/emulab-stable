#!/bin/sh
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#

DIR=/usr/local/etc/emulab
LOG=/var/tmp/cvsup.log
CVSUP=/usr/local/bin/cvsup
SERVER=cvsup.emulab.net

# So that it finds the auth file!
export HOME=$DIR

exec >>${LOG} 2>&1

if [ -f $DIR/supfile -a -x $CVSUP ]; then
	echo -n "Checking for file updates at "
	date
	set -- `$DIR/tmcc bossinfo`
	$CVSUP -h $1 -e -1 -g -L 1 -a $DIR/supfile
fi

exit 0;
