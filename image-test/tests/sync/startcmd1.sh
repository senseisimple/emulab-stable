#!/bin/sh

cd $1

perl -e 'sleep(rand()*30)'
echo 1 > node1up

/usr/testbed/bin/emulab-sync

cat node0up node1up node2up > node1res

