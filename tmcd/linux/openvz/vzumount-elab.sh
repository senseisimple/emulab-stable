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
pat="s/^([a-zA-Z0-9\-]+)(\s+${VEID})(.+)\$/\\1/p"
vnodeid=`sed -n -r -e $pat /var/emulab/vnode.map`
if [ -z $vnodeid ]; then
    echo "No vnodeid found for $VEID in $MYROOT/var/emulab/boot/realname;"
    echo "  cannot kill tmcc proxy!"
    exit 44
fi

kill `cat /var/run/tmccproxy.$vnodeid.pid`
rm -f /var/run/tmccproxy.$vnodeid.pid

exit $RETVAL
