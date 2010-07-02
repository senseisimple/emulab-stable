#!/bin/sh
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#

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
uptime
echo "Running PID $$"
echo "Starting magent on $PLAB_IFACE ($PLAB_IP) Extra arguments: $*"
#exec $AS_ROOT strace $MAGENT_DIR/$MAGENT --interface=$PLAB_IFACE --replay-save=/local/logs/stub.replay $*
exec $AS_ROOT $NETMON_DIR/instrument-standalone.sh $MAGENT_DIR/$MAGENT --interface=$PLAB_IFACE --replay-save=/local/logs/stub.replay $*
