#!/bin/sh

case "$1" in
	start)
		if [ -x /usr/local/sbin/named ]; then
			sleep 1
		        /usr/local/sbin/named -c /etc/namedb/named.conf
		fi
		;;
	stop)
		/usr/bin/killall named > /dev/null 2>&1 && echo -n ' named'
		;;
	*)
		echo ""
		echo "Usage: `basename $0` { start | stop }"
		echo ""
		exit 64
		;;
esac





