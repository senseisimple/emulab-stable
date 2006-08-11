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

#
# Just run it!
#
echo "Running PID $$"
echo "Starting dbmonitor, Extra arguments: $*"
exec $AS_ROOT $DBMONITOR_DIR/$DBMONITOR $* $PID $EID
