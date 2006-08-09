#!/bin/sh
#
# Script to run the monitoring agent
#

#
# Let common-env know what role we're playing
#
export HOST_ROLE="stub"

#
# Grab common environment variables
#
. `dirname $0`/../common-env.sh

#
# Have libnetmon export to a seperate file
#
export LIBNETMON_OUTPUTFILE="/local/logs/libnetmon.out"

#
# Just run the stub!
#
echo "Running PID $$"
echo "Starting magent on $PLAB_IFACE ($PLAB_IP) Extra arguments: $*"
exec $AS_ROOT $NETMON_DIR/instrument-standalone.sh $MAGENT_DIR/$MAGENT --interface=$PLAB_IFACE $*
