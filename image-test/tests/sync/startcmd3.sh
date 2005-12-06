#!/bin/sh

cd $1

# This wil deadlock unless the asynchronous (-a) option is working
/usr/testbed/bin/emulab-sync

sleep 22
echo 3 > node3up

/usr/testbed/bin/emulab-sync -n barrier2

cat node3up node4up > node3res





