#!/bin/sh

. `dirname $0`/../common-env.sh

LIB_SO="libnetmon.so"
SOCK="${TMPDIR}/netmon.sock";

#
# Try once to build the library
#
if ! [ -e $NETMON_DIR/$LIB_SO ]; then
    echo "Building $LIB_SO";
    gmake -C $NETMON_DIR $LIB_SO
fi

#
# Okay, let the user take care of it
#
if ! [ -e $NETMON_DIR/$LIB_SO ]; then
    echo "$NETMON_DIR/$LIB_SO missing - run 'gmake' in $NETMON_DIR to build it"
    exit 1;
fi

#
# Export the magic juice that will make programs use libnetmon
#
export LD_LIBRARY_PATH=$NETMON_DIR
export LD_PRELOAD=$LIB_SO
export LIBNETMON_SOCKPATH=$SOCK
#export LIBNETMON_OUTPUTVERSION=2
