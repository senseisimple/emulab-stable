#!/bin/sh

# Usage: run-client.sh <dest1-ip> <dest2-ip> <port> <base-path> <output-file> <duration>

echo sudo /usr/sbin/tcpdump -n -tt -i vnet dst port $3 | perl $4/glean.pl > /local/logs/$5
sudo /usr/sbin/tcpdump -n -tt -i vnet dst port $3 | perl $4/glean.pl > /local/logs/$5 &
DUMPPID=$!

echo $4/iperf -c $1 -p $3 -t $6
$4/iperf -c $1 -p $3 -t $6&
FIRSTPID=$!

echo $4/iperf -c $2 -p $3 -t $6
$4/iperf -c $2 -p $3 -t $6&
SECONDPID=$!

trap "sudo kill $DUMPPID; sudo kill $FIRSTPID; sudo kill $SECONDPID" EXIT
wait
exit 0

