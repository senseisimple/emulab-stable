#!/bin/sh
#
# vzmount-elab bind mounts all the /users and /proj nfs mounts into the
# container.  Probably we should do better, but it's easy for now.  Note that
# we use the `-n' option to mount, which doesn't place our mounts in mtab
# and lets them be sloppily unmounted when the container is stopped.  
# /proc/mounts still shows them though...
#

if [ -z $VEID ]; then
    echo "Must set VEID env var!"
    exit 33
fi

if [ -z $VE_CONFFILE ]; then
    echo "Must set VE_CONFFILE env var!"
    exit 34
fi

. $VE_CONFFILE

MYROOT=${VE_ROOT}
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
    echo "No vnodeid found for $VEID in /var/emulab/vnode.map;"
    echo "  cannot start tmcc proxy!"
    exit 44
fi

echo "Doing Emulab mounts."
/usr/local/etc/emulab/rc/rc.mounts -j $vnodeid $MYROOT 0 boot
if [ $? = 0 ]; then
    echo "ok."
else
    echo "FAILED with exit code $?"
    exit 44
fi

/usr/local/etc/emulab/tmcc -d -x $MYROOT/var/emulab/tmcc.sock \
    -n $vnodeid -o /var/emulab/logs/tmccproxy.$vnodeid.log >& /dev/null &
echo $! > /var/run/tmccproxy.$vnodeid.pid

echo "/var/emulab/tmcc.sock" > $MYROOT/var/emulab/boot/proxypath

exit $RETVAL
