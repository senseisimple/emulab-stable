#!/bin/sh

# Usage: run-server.sh <port> <base-path> <output-file>

echo sudo /usr/sbin/tcpdump -n -tt -i vnet dst port $1 | perl $2/glean.pl > /local/logs/$3
sudo /usr/sbin/tcpdump -n -tt -i vnet dst port $1 | perl $2/glean.pl > /local/logs/$3 &
DUMPPID=$!

echo $2/iperf -s -p $1
$2/iperf -s -p $1 &
IPERFPID=$!

trap "sudo kill $DUMPPID; sudo kill $IPERFPID" EXIT
wait
exit 0
