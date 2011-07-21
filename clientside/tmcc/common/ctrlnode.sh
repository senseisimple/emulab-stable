#!/bin/sh
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2004 University of Utah and the Flux Group.
# All rights reserved.
#
. /etc/emulab/paths.sh

#
# Boottime initialization of the control node (aka ops, users, etc).
#
case "$1" in
start)
	$BINDIR/rc/rc.ctrlnode -b boot
	;;
stop)
	$BINDIR/rc/rc.ctrlnode shutdown 
	;;
*)
	echo "Usage: `basename $0` {start|stop}" >&2
	;;
esac

exit 0
