#!/bin/sh
#
# Script to run the monitor, collecting data from libnetmon
#

#
# Grab common environment variables
#
. `dirname $0`/../common-env.sh

if [ $# != 1 ]; then
    if [ $# != 2 ]; then
      echo "Usage: $0 <my-ip> [stub-ip]"
      exit 1;
    fi
    SIP=$4
fi

PID=$PROJECT
EID=$EXPERIMENT
MIP=$1
NETMOND=netmond

if ! [ -x "$NETMON_DIR/$NETMOND" ]; then
    gmake -C $NETMON_DIR $NETMOND
fi

if ! [ -x "$NETMON_DIR/$NETMOND" ]; then
    echo "$NETMON_DIR/$NETMOND missing - run 'gmake' in $NETMOND_DIR to build it"
    exit 1;
fi

echo "Starting up netmond for $PID/$EID $MIP $SIP";
$NETMON_DIR/$NETMOND | python $MONITOR_DIR/monitor.py ip-mapping.txt $PID/$EID $MIP $SIP
