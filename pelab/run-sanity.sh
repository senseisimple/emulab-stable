#!/bin/sh
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#

#
# This is really only needed in order to start/stop tcpdump as root...
#

if [ $# -lt 2 ]; then
    echo "usage: run-sanity iface node"
    exit 1
fi
iface=$1
node=$2
if [ $# -eq 4 ]; then
  tdexpr="( ( tcp and dst port $3 ) or ( udp and dst port $4 ) )"
else
  tdexpr="( tcp or udp )"
fi

ARGS="-i $iface -w /local/logs/SanityCheck.log dst host $node and $tdexpr"
echo "Running tcpdump $ARGS"
sudo tcpdump $ARGS &
PID=$!
trap "sudo kill $PID; sudo killall tcpdump" EXIT
wait
exit 0
