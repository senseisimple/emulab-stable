#!/bin/sh

tac=/usr/site/bin/tac

if [ $# -eq 0 ]; then exit 1; fi
host=$1
line=`$tac /usr/testbed/log/powermon.log | grep $host: | head -1`
temp=`echo $line | sed -n -e 's/.*, \([0-9][0-9]*\.*[0-9]*\)F$/\1/p'`
temp=${temp:-'0.0'}
power=`echo $line | sed -n -e 's/.*, \([0-9][0-9]*\.*[0-9]*\)W, .*/\1/p'`
power=${power:-'0.0'}
current=`echo $line | sed -n -e 's/.*: \([0-9][0-9]*\.*[0-9]*\)A, .*/\1/p'`
current=${current:-'0.0'}

echo $temp degrees F
echo $power Watts
echo $current Amps

exit 0
