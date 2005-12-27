#!/bin/sh

if [ $# -eq 0 ]; then exit 1; fi
host=$1
line=`tac /usr/testbed/log/powermon.log | grep $host: | head -1`
temp=`echo $line | sed -n -e 's/.*, \([0-9][0-9]*\.*[0-9]*\)F$/\1/p'`
power=`echo $line | sed -n -e 's/.*, \([0-9][0-9]*\.*[0-9]*\)W, .*/\1/p'`
current=`echo $line | sed -n -e 's/.*: \([0-9][0-9]*\.*[0-9]*\)A, .*/\1/p'`
echo $temp degrees F
echo $power Watts
echo $current Amps
exit 0

