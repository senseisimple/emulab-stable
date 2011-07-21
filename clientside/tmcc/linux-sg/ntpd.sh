#!/bin/sh
#
# Start ntpd - Emulab style.
#

. /etc/emulab/paths.sh

case "$1" in
start)
	if [ -x $BINDIR/ntpstart ]; then
    		$BINDIR/ntpstart /usr/sbin/ntpd && echo " ntpd started"
	fi
	;;

stop)
	killall ntpd
	;;

  *)
        echo "Usage: $0 {start|stop}"
        exit 1
esac

exit 0
