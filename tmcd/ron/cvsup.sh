#!/bin/sh
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#

#
# Fire up a cvsup each time we boot.
#
case "$1" in
start)
	if [ -f /usr/local/etc/emulab/runcvsup.sh ]; then
	    /usr/local/etc/emulab/runcvsup.sh && echo -n ' cvsup'
	fi
	;;
stop)
	;;
*)
	echo "Usage: `basename $0` {start|stop}" >&2
	;;
esac

exit 0
