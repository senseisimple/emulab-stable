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
	echo ""
	$BINDIR/rc.testbed
	;;
stop)
	# Foreground mode.
	$BINDIR/bootvnodes -f -h
	echo "Informing the testbed we're rebooting"
	$BINDIR/tmcc state SHUTDOWN
	;;
*)
	echo "Usage: `basename $0` {start|stop}" >&2
	;;
esac

exit 0
