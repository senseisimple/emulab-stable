#!/bin/sh

ARGS=$*

. `dirname $0`/../common-env.sh

#
# Start up our own measurement agent
#
echo $SH ${MAGENT_DIR}/run-magent.sh $ARGS
$SH ${MAGENT_DIR}/run-magent.sh $ARGS &
MAGENTPID=$!
# Kill the agent if we get killed - TODO: harsher kill?
trap "$AS_ROOT kill $MAGENTPID; $AS_ROOT killall $MAGENT" EXIT

#
# Give it time to come up
#
sleep 1

#
# Wait for all of the agents to start
#
echo "Waiting for measurement agents to become ready";
barrier_wait "stub";

#
# Wait for all the monitors to come up
#
echo "Waiting for monitors to become ready";
barrier_wait "monitor";

echo "Running!";

#
# Wait for our agent to finish
#
wait
