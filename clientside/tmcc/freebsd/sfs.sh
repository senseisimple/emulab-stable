#!/bin/sh

case "$1" in
start)
	if [ ! -e /etc/sfs ]; then
		mkdir -m 0755 /etc/sfs;
	fi
        if [ -f /usr/local/bin/sfskey -a ! -f /etc/sfs/sfs_host_key ]; then
	        /usr/local/bin/sfskey gen -K -P -l \
			 root@`hostname` /etc/sfs/sfs_host_key
        fi
	if [ -x /usr/local/sbin/sfscd ]; then
		/usr/local/sbin/sfscd && echo -n ' sfscd'
	fi
	if [ -x /usr/local/sbin/sfssd ]; then
		/usr/local/sbin/sfssd && echo -n ' sfssd'
	fi
        ;;
stop)
	if [ -f /var/run/sfssd.pid ]; then
		kill `cat /var/run/sfssd.pid`;
	fi
	if [ -f /var/run/sfscd.pid ]; then
		kill `cat /var/run/sfscd.pid`;
	fi
        ;;
*)
        echo "Usage: `basename $0` {start|stop}" >&2
        ;;
esac

exit 0

