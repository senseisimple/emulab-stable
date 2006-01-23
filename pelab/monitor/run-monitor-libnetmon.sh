#!/bin/sh

if [ $# != 2 ]; then
    echo "Usage: $0 pid eid"
    exit 1;
fi

PID=$1
EID=$2

SCRIPT=`which $0`
SCRIPT_LOCATION=`dirname $SCRIPT`
BIN_LOCATION=$SCRIPT_LOCATION/../libnetmon/
BIN=netmond

if ! [ -x $BIN_LOCATION/$BIN ]; then
    echo "$BIN_LOCATION/$BIN missing - run 'gmake' in $BIN_LOCATION to build it"
    exit 1;
fi

echo "Starting up netmond for $PID $EID";
$BIN_LOCATION/$BIN | python monitor.py ip-mapping.txt $PID $EID
