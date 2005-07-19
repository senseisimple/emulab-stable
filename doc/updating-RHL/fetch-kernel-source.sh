#!/bin/bash

set -e

VER=2.6.12-1.1390_FC4
BVER=2.6.12
MVER=2.6
SDIR=ftp://download.fedora.redhat.com/pub/fedora/linux/core/updates/4/SRPMS/
TDIR=/z/src

SRCRPM=kernel-$VER.src.rpm

cd /tmp/
wget $SDIR/$SRCRPM
rpm -Uvh $SRCRPM 
rm $SRCRPM
cd /usr/src/redhat/SPECS
rpmbuild -bp --target $(arch) kernel-$MVER.spec 

cd /usr/src/redhat/BUILD/kernel-$BVER
mv linux-$BVER $TDIR/linux-$VER
rpmbuild --clean --rmsource --rmspec /usr/src/redhat/SPECS/kernel-$MVER.spec
