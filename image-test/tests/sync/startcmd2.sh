#!/bin/sh

cd $1;

# This wil deadlock unless the asynchronous (-a) option is working
/usr/testbed/bin/emulab-sync -a -i 2 -n barrier2

perl -e 'sleep(rand()*30)'
echo 2 > node2up

/usr/testbed/bin/emulab-sync

cat node0up node1up node2up > node2res

