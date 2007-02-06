#!/bin/sh
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#

ARGS=$*

. `dirname $0`/../common-env.sh

#
# cd to the log directory so that any files that get written - cores, gprof
# files, etc. get put in a place where loghole will see them
#
cd $LOGDIR

TARGETS=$MAGENT

#
# Start up our own measurement agent
#
if [ ${MAGENT_NORECV:-0} -ne 0 ]; then
    port=`echo $ARGS | sed -e 's/.*--peerserverport=\([0-9][0-9]*\).*/\1/'`
    ARGS=`echo $ARGS | sed -e "s/--peerserverport=$port/--peerserverport=0/"`
    echo "${IPERFD_DIR}/$IPERFD -p $port"
    ${IPERFD_DIR}/$IPERFD -p $port &
    TARGETS="$TARGETS $IPERFD"
fi

echo $SH ${MAGENT_DIR}/run-magent.sh $ARGS
$SH ${MAGENT_DIR}/run-magent.sh --daemonize $ARGS 
# Kill the agent if we get killed - TODO: harsher kill?
# Because the magent backgrounds itself, it's harder to figure out
# what its pid is, just just do a killall
# Note that we assume that a kill of us is "normal" and just exit 0.
trap "$AS_ROOT killall $TARGETS; exit 0" TERM

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
# We just sleep forever here as the stub has detached itself and
# we cannot exit without screwing up the program agent.  The program
# agent will kill us (and the magent by virtue of the trap) when it
# is good and ready.
#
while true; do sleep 1000; done
exit 0
