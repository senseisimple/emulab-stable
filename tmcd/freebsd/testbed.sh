#!/bin/sh
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#

#
# Boottime initialization. 
#
case "$1" in
start)
	if [ -x /etc/testbed/rc.testbed ]; then
	    echo ""
	    /etc/testbed/rc.testbed
	fi
	;;
stop)
	;;
*)
	echo "Usage: `basename $0` {start|stop}" >&2
	;;
esac

exit 0
