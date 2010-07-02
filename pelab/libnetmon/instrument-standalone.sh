#!/bin/sh
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#

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
# Make sure we can get coredumps
#
ulimit -c unlimited

#
# Get a new version of the output
#
export LIBNETMON_OUTPUTVERSION=3
export LIBNETMON_MONITORUDP=1

echo "Instrumenting '$@' with libnetmon"
exec $@
