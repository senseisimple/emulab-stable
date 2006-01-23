#!/bin/sh
SCRIPT=`which $0`
SCRIPT_LOCATION=`dirname $SCRIPT`
LIB_LOCATION=$SCRIPT_LOCATION/../libnetmon/
LIB_SO=libnetmon.so
SOCK=/var/tmp/netmon.sock
if ! [ -x $LIB_LOCATION/$LIB_SO ]; then
    echo "$LIB_LOCATION/$LIB_SO missing - run 'gmake' in $LIB_LOCATION to build it"
    exit 1;
fi

export LD_LIBRARY_PATH=$LIB_LOCATION
export LD_PRELOAD=$LIB_SO
export LIBNETMON_SOCKPATH=$SOCK

echo "Instrumenting '$@' with libnetmon"
$@
