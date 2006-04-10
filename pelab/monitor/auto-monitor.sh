#!/bin/sh

. `dirname $0`/../common-env.sh

#
# Wait for all of the stubs to start
#
echo "Waiting for stubs to become ready";
barrier_wait "stub";

#
# Start up our own monitor
#
$SH ${MONITOR_DIR}/run-monitor-libnetmon.sh $* &
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
barrier_wait "monitor";

echo "Running!";

#
# Wait for our monitor to finish
#
wait
