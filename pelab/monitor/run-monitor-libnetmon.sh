#!/bin/sh
LIB_LOCATION=../libnetmon/
LIB_SO=libnetmon.so
if ! [ -x $LIB_LOCATION/$LIB_SO ]; then
    echo "$LIB_LOCATION/$LIB_SO missing - run 'gmake' in $LIB_LOCATION to build it"
    exit 1;
fi

export LD_LIBRARY_PATH=$LIB_LOCATION
export LD_PRELOAD=$LIB_SO

PID=shift
EID=shift

echo "$0 running $@";
$@ | python monitor.py ip-mapping.txt $PID $EID
