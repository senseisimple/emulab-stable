#!/bin/sh

# Usage: run-client.sh <dest1-ip> <dest2-ip> <port> <base-path> <output-file> <duration>

sudo /usr/sbin/tcpdump -i vnet dst port $3 > /local/logs/$5 &
$4/iperf -c $1 -p $3 -t $6&
$4/iperf -c $2 -p $3 -t $6
