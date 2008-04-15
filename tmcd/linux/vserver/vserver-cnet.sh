#!/bin/sh
#
# (De)configure vserver interfaces: loopback and control net.
# This is installed as: /etc/rc3.d/S00cnet on RHL based vservers.
#

action=$1

. /etc/init.d/functions
. /etc/emulab/paths.sh

RETVAL=0

if [ ! -e $BOOTDIR/controlif -o ! -e $BOOTDIR/myip -o \
     ! -e $BOOTDIR/mynetmask -o ! -e $BOOTDIR/routerip ]; then
	echo "vserver pid $$: no control net params!?"
	exit 1
fi
cnetdev=`cat $BOOTDIR/controlif`
cnetip=`cat $BOOTDIR/myip`
cnetmask=`cat $BOOTDIR/mynetmask`
cnetrouter=`cat $BOOTDIR/routerip`

case "$action" in
start)
    echo -n "Bringing up control net interface $netdev: "
    ifconfig $cnetdev $cnetip netmask $cnetmask || {
	echo "vserver pid $$: could not bring up cnet interface"
	echo_failure
	exit 1
    }
    route add default gw $cnetrouter || {
	echo "vserver pid $$: could not add default route"
	echo_failure
	exit 1
    }
    ifconfig lo up
    echo_success
    ;;
stop)
    echo -n "Shutting down control net interface $netdev: "
    ifconfig lo down
    ifconfig $cnetdev down
    echo_success
    ;;
*)
    echo "Usage: `basename $0` {start|stop}" >&2
    RETVAL=1
    ;;
esac
exit $RETVAL


