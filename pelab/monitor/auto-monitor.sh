#!/bin/sh

ARGS=$*

. `dirname $0`/../common-env.sh

#
# Wait for all of the stubs to start
#
echo "Waiting for stubs to become ready";
barrier_wait "stub"; _rval=$?
if [ $_rval -ne 0 ]; then
    echo "*** WARNING: not all stubs started ($_rval)"
fi

#
# Potential race condition here? The monitor cannot connect to the
# stub, and the problem is flaky -JD
#
sleep 1

#
# Start up our own monitor
#
echo $SH ${MONITOR_DIR}/run-monitor-libnetmon.sh $ARGS
$SH ${MONITOR_DIR}/run-monitor-libnetmon.sh $ARGS &
MONPID=$!
# Kill the monitor if we get killed - TODO: harsher kill?
trap "$AS_ROOT kill $MONPID; $AS_ROOT killall netmond" EXIT

#
# Give it time to come up
#
sleep 1

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
