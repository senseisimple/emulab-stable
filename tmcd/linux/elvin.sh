#!/bin/sh

# Source function library.
. /etc/rc.d/init.d/functions

# Source networking configuration.
if [ ! -f /etc/sysconfig/network ]; then
    exit 0
fi

. /etc/sysconfig/network

# Check that networking is up.
[ ${NETWORKING} = "no" ] && exit 0

case "$1" in
start)
	if [ -x /usr/local/sbin/elvind ]; then
		/usr/local/sbin/elvind && echo -n ' elvind'
	fi
  	touch /var/lock/subsys/elvin
        ;;
stop)
	killproc elvind
  	rm -f /var/lock/subsys/elvin
        ;;
*)
        echo "Usage: `basename $0` {start|stop}" >&2
        ;;
esac

exit 0

