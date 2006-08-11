#!/bin/sh

ARGS=$*
if [ -z "$PID" -o -z "$EID" ]; then
    echo "*** experiment PID and EID are not set!"
    exit 1
fi

. `dirname $0`/../common-env.sh

#
# We don't give a rat's ass about the stubs, but we have to stay in synch
#
echo "Waiting for stubs to become ready";
barrier_wait "stub"; _rval=$?
if [ $_rval -ne 0 ]; then
    echo "*** WARNING: not all stubs started ($_rval)"
fi

#
# Copy over the node list
#
cp -p /proj/$PID/exp/$EID/tmp/node_list /var/tmp/node-mapping

#
# Start up our own monitor
#
echo $SH ${DBMONITOR_DIR}/run-dbmonitor.sh $ARGS
$SH ${DBMONITOR_DIR}/run-dbmonitor.sh $ARGS &
DBMONPID=$!
# Kill the monitor if we get killed - TODO: harsher kill?
trap "$AS_ROOT kill $DBMONPID" EXIT

#
# Give it time to come up
#
sleep 1

#
# Wait for all the monitors to come up
#
echo "Waiting for dbmonitors to become ready";
barrier_wait "monitor"; _rval=$?
if [ $_rval -ne 0 ]; then
    echo "*** WARNING: not all dbmonitors started ($_rval)"
fi

echo "Running!";

#
# Wait for our monitor to finish
#
wait
