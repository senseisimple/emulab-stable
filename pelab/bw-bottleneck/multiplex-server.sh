#!/bin/sh

# Usage: multiplex-server.sh <port> <base-path>

echo $2/iperf -s -p $1 '&'
$2/iperf -s -p $1 &
IPERFPID=$!

trap "sudo kill $IPERFPID" EXIT
wait
exit 0
