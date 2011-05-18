#!/bin/sh

# fake frisbee uploader
# frisuploader -i 10.200.1.252 -T 900 -s 10000000 -m 10.200.1.1 -p 1234 outfile

log="/usr/testbed/log/uploader.log"

shift 1; iface=$1
shift 2; timeo=$1
shift 2; size=$1
shift 2; client=$1
shift 2; port=$1
shift 1; outfile=$1

echo "`date`: frisuploader: from ${client}:${port} on iface $iface to $outfile (size=$size, timo=$timeo)" >> $log

sleep 5
exit 0
