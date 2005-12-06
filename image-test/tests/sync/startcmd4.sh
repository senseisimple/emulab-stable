#!/bin/sh

cd $1

echo 4 > node4up

/usr/testbed/bin/emulab-sync -n barrier2

cat node3up node4up > node4res


