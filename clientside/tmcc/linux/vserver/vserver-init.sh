#!/bin/sh
#
# First script run in the new vserver (from mkvserver.pl).
# We write our pid to a file informing the "outside" that we now exist.
# This triggers the insertion of network interfaces into the vserver.
# We wait here til we get the "all clear" flag and then continue.
#
# XXX there has got to be an easier way...
#

action=$1

. /etc/init.d/functions
. /etc/emulab/paths.sh

RETVAL=0
case "$action" in
start)
    rm -f $BOOTDIR/ready $BOOTDIR/vserver.pid
    exec $BINDIR/vserver-rc 3 6
    ;;
stop)
    if [ -r $BOOTDIR/ready ]; then
	$BINDIR/vserver-rc 6 3   
    fi
    ;;
*)
    echo "No can do action \"$1\" bubba"
    RETVAL=1
    ;;
esac
exit $RETVAL
