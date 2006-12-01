#!/bin/sh
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#

if [ $# -lt 2 ]; then
    echo "usage: run-tcpdump type index [ tcpdump-args ... ]"
    exit 1
fi

kind=$1
if [ $kind = "planet" ]; then
    echo "cannot do real planetlab nodes yet"
    exit 1
fi

ix=$2

shift; shift
ARGS=$*

findif="/proj/tbres/bin/findif-linux"
if [ ! -x $findif ]; then
    echo "need $findif"
    exit 1
fi

case $kind in
elab)
    ip="10.0.0.$ix"
    iface=`$findif -i $ip`
    ;;
plab)
    ip="10.1.0.$ix"
    iface=`$findif -i $ip`
    ;;
*)
    echo "unrecognized node type $kind"
    exit 1
    ;;
esac

if [ -z "$ip" ]; then
    echo "Could not find interface for IP $ip"
    exit 1
fi

ARGS="-i $iface -w /local/logs/$kind-$ix.tcpdump $ARGS"
echo "$kind-${ix}: Running tcpdump $ARGS"
sudo tcpdump $ARGS &
PID=$!
trap "sudo kill $PID; sudo killall tcpdump" EXIT
wait
exit 0
