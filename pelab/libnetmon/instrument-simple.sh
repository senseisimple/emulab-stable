#!/bin/sh
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#

export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH
export LD_PRELOAD=libnetmon.so
export LIBNETMON_MONITORUDP=1
if [ "x$LIBNETMON_OUTPUTVERSION" = "x" ]; then
    export LIBNETMON_OUTPUTVERSION=3
else
    export LIBNETMON_OUTPUTVERSION
fi

$*
