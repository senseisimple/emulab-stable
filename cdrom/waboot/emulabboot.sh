#!/bin/sh
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#

#
# This script goes in /usr/local/etc/rc.d on the disk image.
#
# Set the magic bit that said we booted from the disk okay. Otherwise
# if the CDROM boots and this has not been done, the CDROM assumes the
# disk is scrogged.
# 
. /etc/rc.conf.local

#
# netbed_disk is set in rc.conf.local by the CDROM boot image.
#
case "$1" in
start)
	if [ -x /usr/site/sbin/tbbootconfig ]; then
		/usr/site/sbin/tbbootconfig -c 1 $netbed_disk

		case $? in
		0)
		    exit 0
		    ;;
		*)
		    echo 'Error in testbed boot header program'
		    echo 'Reboot failed. HELP!'
		    exit 1
		    ;;
		esac
	fi
	;;
stop)
	;;
*)
	echo "Usage: `basename $0` {start|stop}" >&2
	exit 1
	;;
esac
