#!/bin/sh
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#

#
# This file goes in /usr/local/etc/rc.d on the CDROM.
#
# Get the disk and pass that to the registration program. It does
# all the actual work.
# 

. /etc/rc.conf.local

case "$1" in
start)
	if [ -f /usr/site/sbin/register.pl ]; then
		/usr/site/sbin/register.pl $netbed_disk $netbed_IP
		exit $?
	fi
	;;
stop)
	;;
*)
	echo "Usage: `basename $0` {start|stop}" >&2
	exit 1
	;;
esac
