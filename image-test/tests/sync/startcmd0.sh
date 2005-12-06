#!/bin/sh

cd $1

perl -e 'sleep(rand()*30)'
echo 0 > node0up

/usr/testbed/bin/emulab-sync -i 3

cat node0up node1up node2up > node0res

