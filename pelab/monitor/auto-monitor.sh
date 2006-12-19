#!/bin/sh
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#

ARGS=$*

. `dirname $0`/../common-env.sh

#
# How long to wait for netmond.
#
NETMOND_TIMO=60

#
# Wait for all of the stubs to start
#
echo "Waiting for stubs to become ready";
barrier_wait "stub"; _rval=$?
if [ $_rval -ne 0 ]; then
    echo "*** WARNING: not all stubs started ($_rval)"
fi

#
# Start up our own monitor
#
echo $SH ${MONITOR_DIR}/run-monitor-libnetmon.sh $ARGS
$SH ${MONITOR_DIR}/run-monitor-libnetmon.sh $ARGS &
MONPID=$!
# Kill the monitor if we get killed - TODO: harsher kill?
# Note that we assume that a kill of us is "normal" and just exit 0.
trap "{ $AS_ROOT kill $MONPID; $AS_ROOT killall netmond; true; }" TERM

#
# Make sure it is ready to receive requests.
# We do this with a simple program that just opens a socket.
# When run under libnetmon, it will fail if netmond is not yet running.
#
if [ -x ${NETMON_DIR}/netmonup ]; then
    LIBNETMON_CONNECTTIMO=$NETMOND_TIMO \
	${MONITOR_DIR}/instrument.sh ${NETMON_DIR}/netmonup
    if [ $? -ne 0 ]; then
        echo "**** WARNING: netmond failed to start after $NETMOND_TIMO seconds"
    fi
else
    sleep 2
fi

#
# Wait for all the monitors to come up
#
echo "Waiting for monitors to become ready";
barrier_wait "monitor"; _rval=$?
if [ $_rval -ne 0 ]; then
    echo "*** WARNING: not all monitors started ($_rval)"
fi

echo "Running!";

#
# Wait for our monitor to finish
#
wait
status=$?
if [ $status -eq 15 ]; then
    status=0
fi
exit $status
