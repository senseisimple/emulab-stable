#!/bin/sh

# Usage: multiplex-client.sh <port> <base-path> <dest> <duration> <dest-file>

echo `date` > /local/logs/$5.time

echo sudo /usr/sbin/tcpdump -n -tt -i vnet dst port $1 '|' perl $2/count-retransmits.pl '>' /local/logs/$5.retransmit '&'
sudo /usr/sbin/tcpdump -n -tt -i vnet dst port $1 | perl $2/count-retransmits.pl > /local/logs/$5.retransmit &
DUMPPID=$!

echo perl $2/send-multiplex.pl $3 $1 $4 '>&' /local/logs/$5.graph '&'
perl $2/send-multiplex.pl $3 $1 $4 >& /local/logs/$5.graph &
IPERFPID=$!

trap "sudo kill $DUMPPID; sudo kill $IPERFPID" EXIT
wait
exit 0

