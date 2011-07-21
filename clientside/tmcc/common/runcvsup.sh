#!/bin/sh
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#

. /etc/emulab/paths.sh

LOG=$LOGDIR/cvsup.log
CVSUP=/usr/local/bin/cvsup

# So that cvsup finds the auth file!
export HOME=/root

if [ -f $ETCDIR/supfile -a -x $CVSUP ]; then
	#
	# Redirect to log file on remote nodes.
	#
	if [ -e $ETCDIR/isrem ]; then
		exec >>${LOG} 2>&1
	fi
	
	echo -n "Checking for file updates at "
	date
	# Do nothing.
	if [ -e $ETCDIR/nosup ]; then
		echo "CVSUP is currently turned off ..."
		exit 0
	fi
	set -- `$BINDIR/tmcc bossinfo`
	supserver=$1
	$CVSUP -h $supserver -e -1 -g -L 1 -a $ETCDIR/supfile

	#
	# Per machine cvsup based on nodeid, but only on remote nodes.
	#
	if [ ! -e $ETCDIR/isrem ]; then
		exit 0
	fi
	set -- `$BINDIR/tmcc nodeid`
	case $? in
	0)
	    if [ "$1" != "" ]; then 
		rm -f /tmp/sup.$$
		cat $ETCDIR/supfile | \
  		  sed -e "s/release=[a-zA-Z0-9]*/release=$1/" >/tmp/sup.$$
		$CVSUP -h $supserver -e -1 -g -L 1 -a /tmp/sup.$$
	        rm -f /tmp/sup.$$
	    fi
	    ;;
	esac
fi

exit 0;
