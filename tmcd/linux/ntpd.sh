#!/bin/sh
#
# ntpd		This shell script takes care of starting and stopping
#		ntpd (NTPv4 daemon).
#
# chkconfig: - 26 74
# description: ntpd is the NTPv4 daemon.

# Source function library.
. /etc/init.d/functions

# Source networking configuration.
. /etc/sysconfig/network

# Check that networking is up.
[ ${NETWORKING} = "no" ] && exit 0

[ -x /usr/sbin/ntpd -a -f /etc/ntp.conf ] || exit 0

. /etc/emulab/paths.sh

if [ -x $BINDIR/ntpstart ]; then
    NTPD="--check ntpd $BINDIR/ntpstart /usr/sbin/ntpd";
else 
    NTPD="ntpd";
fi

RETVAL=0
prog="ntpd"

start() {
	# Adjust time to make life easy for ntpd
	if [ -f /etc/ntp/step-tickers ]; then
		echo -n $"Synchronizing with time server: "
		/usr/sbin/ntpdate -s -b -p 8 \
		    `/bin/sed -e 's/#.*//' /etc/ntp/step-tickers`
		success
		echo
	fi
        # Start daemons.
        echo -n $"Starting $prog: "
        daemon $NTPD
	RETVAL=$?
        echo
        [ $RETVAL -eq 0 ] && touch /var/lock/subsys/ntpd
	return $RETVAL
}

stop() {
        # Stop daemons.
        echo -n $"Shutting down $prog: "
	killproc ntpd
	RETVAL=$?
        echo
        [ $RETVAL -eq 0 ] && rm -f /var/lock/subsys/ntpd
	return $RETVAL
}

# See how we were called.
case "$1" in
  start)
	start
        ;;
  stop)
	stop
        ;;
  status)
	status ntpd
	RETVAL=$?
	;;
  restart|reload)
	stop
	start
	RETVAL=$?
	;;
  condrestart)
	if [ -f /var/lock/subsys/ntpd ]; then
	    stop
	    start
	    RETVAL=$?
	fi
	;;
  *)
        echo $"Usage: $0 {start|stop|restart|condrestart|status}"
        exit 1
esac

exit $RETVAL
