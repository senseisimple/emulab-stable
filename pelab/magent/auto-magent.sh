#!/bin/sh

ARGS=$*

. `dirname $0`/../common-env.sh

#
# cd to the log directory so that any files that get written - cores, gprof
# files, etc. get put in a place where loghole will see them
#
cd $LOGDIR

#
# Start up our own measurement agent
#
echo $SH ${MAGENT_DIR}/run-magent.sh #$ARGS
$SH ${MAGENT_DIR}/run-magent.sh --daemonize #$ARGS 
# Kill the agent if we get killed - TODO: harsher kill?
# Because the magent backgrounds itself, it's harder to figure out
# what its pid is, just just do a killall
# Note that we assume that a kill of us is "normal" and just exit 0.
trap "$AS_ROOT killall $MAGENT; exit 0" TERM

#
# Give it time to come up
#
sleep 1

#
# Wait for all of the agents to start
#
echo "Waiting for measurement agents to become ready";
barrier_wait "stub"; _rval=$?
if [ $_rval -ne 0 ]; then
    echo "*** WARNING: not all stubs started ($_rval)"
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
# XXX ignores exit status of child
#
wait
exit 0
