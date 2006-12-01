#!/bin/sh
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#

#
# Load in the appropriate environment variables
#
. `dirname $0`/instrument-env.sh

echo "Instrumenting '$@' with libnetmon"
exec $@
