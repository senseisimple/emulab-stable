#!/bin/sh

case "$1" in
start)
	# Need to compensate for some weird race that's happening.
	sleep 5
	/usr/local/sbin/elvind && echo " elvind started."
        ;;
stop)
	killall elvind
        ;;
*)
        echo "Usage: `basename $0` {start|stop}" >&2
        ;;
esac

exit 0

