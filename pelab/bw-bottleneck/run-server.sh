#!/bin/sh

# Usage: run-server.sh <port> <base-path> <output-file>

sudo /usr/sbin/tcpdump -n -tt -i vnet dst port $1 | perl $2/glean.pl > /local/logs/$3 &
$2/iperf -s -p $1
