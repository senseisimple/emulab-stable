#!/bin/sh
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#
. /etc/emulab/paths.sh

#
# Boottime initialization. 
#
case "$1" in
start)
	if [ -f $BINDIR/emulabctl ]; then
	    $BINDIR/emulabctl start && echo -n ' Emulab'
	fi
	;;
stop)
	if [ -f $BINDIR/emulabctl ]; then
	    $BINDIR/emulabctl stop && echo -n ' Emulab'
	fi
	;;
restart)
	if [ -f $BINDIR/emulabctl ]; then
	    $BINDIR/emulabctl stop
	    echo 'Sleeping a bit before restarting ...'
	    sleep 10
	    $BINDIR/emulabctl start
	fi
	;;
*)
	echo "Usage: `basename $0` {start|stop|restart}" >&2
	;;
esac

exit 0
