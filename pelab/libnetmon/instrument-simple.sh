#!/bin/sh
export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH
export LD_PRELOAD=libnetmon.so
if [ "x$LIBNETMON_OUTPUTVERSION" = "x" ]; then
    export LIBNETMON_OUTPUTVERSION=3
else
    export LIBNETMON_OUTPUTVERSION
fi

$*
