#!/bin/sh

case "$1" in
	start)
		if [ -x /usr/local/libexec/elvind ]; then
		        /usr/local/libexec/elvind
		fi
		;;
	stop)
		/usr/bin/killall elvind > /dev/null 2>&1 && echo -n ' elvind'
		;;
	*)
		echo ""
		echo "Usage: `basename $0` { start | stop }"
		echo ""
		exit 64
		;;
esac





