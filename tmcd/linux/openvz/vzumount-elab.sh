#!/bin/sh

if [ -z $VEID ]; then
    echo "Must set VEID env var!"
    exit 33
fi

MYROOT=/vz/root/${VEID}
if [ ! -e $MYROOT ]; then
    echo "root dir $MYROOT doesn't seem to be mounted!"
    exit 1
fi

RETVAL=0

#
# This is an utter disgrace.  OpenVZ does not called a prestart or start script
# in the root context before booting a container, so this is the only place we
# can perform such actions.
#
# Find our vnode_id:
vnodeid=`cat /var/emulab/vms/vnode.${VEID}`
if [ -z $vnodeid ]; then
    echo "No vnodeid found for $VEID in $MYROOT/var/emulab/boot/realname;"
    echo "  cannot kill tmcc proxy!"
    exit 44
fi

#
echo "Undoing Emulab mounts."
/usr/local/etc/emulab/rc/rc.mounts -j $vnodeid $MYROOT 0 shutdown
if [ $? = 0 ]; then
    echo "ok."
else
    echo "FAILED with exit code $?"
    exit 44
fi

kill `cat /var/run/tmccproxy.$vnodeid.pid`
rm -f /var/run/tmccproxy.$vnodeid.pid

exit $RETVAL
