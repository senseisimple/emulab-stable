#!/bin/sh
#
# Gak we have to do another level of clone to get into an environment with
# virtualized networking.  This is set to be the first and only script run
# when a vserver starts.
#

action=$1

. /etc/init.d/functions
. /etc/emulab/paths.sh

RETVAL=0
case "$action" in
start)
    $BINDIR/chpid -i -h -n /bin/sh $BINDIR/vserver/vserver1.sh
    RETVAL=$?
    ;;
stop)
    # no idea what to do here yet
    kill 0
    ;;
*)
    echo "No can do action \"$1\" bubba"
    RETVAL=1
    ;;
esac
exit $RETVAL
