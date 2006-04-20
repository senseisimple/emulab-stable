#!/bin/sh

ARGS=$*

. `dirname $0`/../common-env.sh

#
# Start up our own stub
#
echo $SH ${STUB_DIR}/run-stub.sh $ARGS
$SH ${STUB_DIR}/run-stub.sh $ARGS &
STUBPID=$!
# Kill the stub if we get killed - TODO: harsher kill?
trap "$AS_ROOT kill $STUBPID; $AS_ROOT killall stubd" EXIT

#
# Give it time to come up
#
sleep 1

#
# Wait for all of the stubs to start
#
echo "Waiting for stubs to become ready";
barrier_wait "stub";

#
# Wait for all the monitors to come up
#
echo "Waiting for monitors to become ready";
barrier_wait "monitor";

echo "Running!";

#
# Wait for our stub to finish
#
wait
