#!/bin/sh
#
# Script to run the stub
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
# Just run the stub!
#
echo "Starting stubd on $PLAB_IFACE ($PLAB_IP) Extra arguments: $*"
exec $AS_ROOT $STUB_DIR/$STUBD $PLAB_IFACE $*
