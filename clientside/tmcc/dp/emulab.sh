#!/bin/sh
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
PATH=/sbin:/bin:/usr/sbin:/usr/bin
[ -f /etc/default/rcS ] && . /etc/default/rcS
. /lib/lsb/init-functions

. /etc/emulab/paths.sh

case "$1" in
    start)
	if [ -x $BINDIR/emulabctl ]; then
	    log_daemon_msg "Starting Emulab services" ""
	    $BINDIR/emulabctl start
	    log_end_msg $?
	fi
        ;;
    reload|force-reload)
        echo "Error: argument '$1' not supported" >&2
        exit 3
        ;;
    stop)
	if [ -x $BINDIR/emulabctl ]; then
	    log_daemon_msg "Stopping Emulab services" ""
	    $BINDIR/emulabctl stop
	    log_end_msg $?
	fi
        ;;
    restart)
	$0 stop && sleep 10 && $0 start
	;;
    *)
        echo "Usage: $0 start|stop|restart" >&2
        exit 3
        ;;
esac

exit 0
