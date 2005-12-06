#!/bin/sh

cd $1

sleep 55
echo 3 > node3up

/usr/testbed/bin/emulab-sync -n barrier2

cat node3up node4up > node3res





