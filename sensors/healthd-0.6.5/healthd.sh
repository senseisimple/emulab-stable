#!/bin/sh

PREFIX=/usr/local

case "$1" in
	start)
                [ -x ${PREFIX}/sbin/h2init ] && ${PREFIX}/sbin/h2init 
		[ -x ${PREFIX}/sbin/healthd ] \
                && ${PREFIX}/sbin/healthd -S -p boss 180 \
                && echo -n ' healthd'
		;;
	stop)
		/usr/bin/killall healthd > /dev/null 2>&1 && echo -n ' healthd'
		;;
	reload)
		/usr/bin/killall -HUP healthd > /dev/null 2>&1 && echo -n ' healthd'
		;;
	*)
		echo ""
		echo "Usage: `basename $0` { start | stop | reload }"
		echo ""
		exit 1
		;;
esac
