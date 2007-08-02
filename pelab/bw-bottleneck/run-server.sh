#!/bin/sh

# Usage: run-server.sh <port> <base-path> <output-file>

sudo /usr/sbin/tcpdump -i vnet dst port $1 > /local/logs/$3 &
$2/iperf -s -p $1
