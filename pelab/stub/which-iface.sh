#!/usr/bin/sh
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#

LINKNAME=planetc
HOSTNAME=`cat /var/emulab/boot/nickname | sed 's/\..*//'`
LINK="$HOSTNAME-$LINKNAME"
LINKIP=`grep "$LINK" /etc/hosts | cut -f1`
IFACENAME=`ifconfig -a | perl -e 'while(<>) { if (/^(eth\d+)/) { $if = $1; } if (/10.1/) { print "$if\n"; exit 0; }} exit 1;'`

echo $IFACENAME
