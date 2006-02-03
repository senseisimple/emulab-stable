#!/bin/sh

#
# Load in the appropriate environment variables
#
. `dirname $0`/instrument-env.sh

echo "Instrumenting '$@' with libnetmon"
exec $@
