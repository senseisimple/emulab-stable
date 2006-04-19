#!/bin/sh

#
# Load in the appropriate environment variables
#
. `dirname $0`/../monitor/instrument-env.sh

#
# Blank a couple out so that we don't try to talk to netmond
#
export -n LIBNETMON_SOCKPATH
export -n LIBNETMON_CONTROL_SOCKPATH

#
# Get a new version of the output
#
export LIBNETMON_OUTPUTVERSION=2

echo "Instrumenting '$@' with libnetmon"
exec $@
