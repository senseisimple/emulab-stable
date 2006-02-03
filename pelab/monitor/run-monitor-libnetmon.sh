#!/bin/sh
#
# Script to run the monitor, collecting data from libnetmon
#

#
# Let common-env know what role we're playing
#
export HOST_ROLE="monitor"

#
# Grab common environment variables
#
. `dirname $0`/../common-env.sh

if [ $# != 0 ]; then
    if [ $# != 1 ]; then
      echo "Usage: $0 [stub-ip]"
      exit 1;
    fi
    SIP=$1
fi

if ! [ -x "$NETMON_DIR/$NETMOND" ]; then
    gmake -C $NETMON_DIR $NETMOND
fi

if ! [ -x "$NETMON_DIR/$NETMOND" ]; then
    echo "$NETMON_DIR/$NETMOND missing - run 'gmake' in $NETMOND_DIR to build it"
    exit 1;
fi

echo "Starting up netmond for $PROJECT/$EXPERIMENT $PELAB_IP $SIP";
exec $NETMON_DIR/$NETMOND | $PYTHON $MONITOR_DIR/$MONITOR ip-mapping.txt $PROJECT/$EXPERIMENT $PELAB_IP $SIP
