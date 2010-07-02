#!/bin/sh
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#

ARGS=$*

. `dirname $0`/../common-env.sh

#
# Start up our own stub
#
echo $SH ${STUB_DIR}/run-stub.sh $ARGS
$SH ${STUB_DIR}/run-stub.sh $ARGS &
STUBPID=$!
# Kill the stub if we get killed - TODO: harsher kill?
# Note that we assume that a kill of us is "normal" and just exit 0.
trap "{ $AS_ROOT kill $STUBPID; $AS_ROOT killall stubd; true; }" TERM

#
# Give it time to come up
#
sleep 1

#
# Wait for all of the stubs to start
#
echo "Waiting for stubs to become ready";
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
status=$?
if [ $status -eq 15 ]; then
    status=0
fi
exit $status
