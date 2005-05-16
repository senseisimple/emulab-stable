#! /bin/sh

cd /etc/emulab

while /bin/true; do
    $BINDIR/garcia-pilot -d -l /var/emulab/logs/pilot.log
    sleep 2
done
