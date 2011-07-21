#!/bin/sh
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2004, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
# PROVIDE: testbed
# REQUIRE: pubsub
#

. /etc/emulab/paths.sh

#
# Boottime initialization. 
#
case "$1" in
start|faststart)
	echo ""
	$BINDIR/rc/rc.testbed
	;;
stop)
	# Foreground mode.
	$BINDIR/rc/rc.bootsetup shutdown
	;;
*)
	echo "Usage: `basename $0` {start|stop}" >&2
	;;
esac

exit 0
