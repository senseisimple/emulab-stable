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

MYROOT=/vz/root/${VEID}
if [ ! -e $MYROOT ]; then
    echo "root dir $MYROOT doesn't seem to be mounted!"
    exit 1
fi

RETVAL=0

__vzmount() {
    mount -t nfs | sed -n -r -e 's/^.*on ([^ ]*) .*$/\1/p' | grep $1 | \
	while read point; \
	do \
            mkdir -p "${MYROOT}${point}"
            echo -n "Mounting $point at ${MYROOT}${point}... "
            mount -n -o bind $point "${MYROOT}${point}"
	    if [ $? = 0 ]; then
		echo "ok."
	    else
		echo "FAILED with exit code $?"
		RETVAL=1
	    fi
        done
}

#
# Grab our list of /users mounts:
#
__vzmount '/users'

#
# Same deal with /proj:
#
__vzmount '/proj'

#
# Quick /share:
#
__vzmount '/share'

#
# This is an utter disgrace.  OpenVZ does not called a prestart or start script
# in the root context before booting a container, so this is the only place we
# can perform such actions.
#
# Find our vnode_id:
pat="s/^([a-zA-Z0-9\-]+)(\s+${VEID})(.+)\$/\\1/p"
vnodeid=`sed -n -r -e $pat /var/emulab/vnode.map`
if [ -z $vnodeid ]; then
    echo "No vnodeid found for $VEID in /var/emulab/vnode.map;"
    echo "  cannot start tmcc proxy!"
    exit 44
fi

/usr/local/etc/emulab/tmcc -d -x $MYROOT/var/emulab/tmcc.sock \
    -n $vnodeid -o /var/emulab/logs/tmccproxy.$vnodeid.log >& /dev/null &
echo $! > /var/run/tmccproxy.$vnodeid.pid

echo "/var/emulab/tmcc.sock" > $MYROOT/var/emulab/boot/proxypath

exit $RETVAL
