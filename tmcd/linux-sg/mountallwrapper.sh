#!/bin/sh
#
# Modified filesys mount hack originally from the Platx distribution
#

# Do the real work of mountall
/etc/init.d/mountall.sh

echo Done with default mountall.

# Make space for /var in ramfs
if [ ! -d /mnt/ramfs/var ]; then
    mkdir /mnt/ramfs/var
fi

if [ ! -d /mnt/ramfs/tmp ]; then
    mkdir /mnt/ramfs/tmp
    chmod 777 /mnt/ramfs/tmp
fi

if [ ! -L /var ]; then
    # Keep a record of the directories needed by /var since /var is going away.
    # This hack replaces the previous hack of deleting /var at build time.
    # This really only does something the first boot of a file system.
    mkdir /mnt/holdvar
    cp -R /var/* /mnt/holdvar

    # Put /var on ramfs
    rm -rf /var
    ln -s /mnt/ramfs/var /var
    rm -rf /tmp
    ln -s /mnt/ramfs/tmp /tmp
fi

# Copy needed /var contents to ramfs /var
cp -R /mnt/holdvar/* /var

# Add additional directories (Some may not need to be done.)
mkdir -p /var/lib/misc
mkdir -p /var/local
mkdir -p /var/log/kernel
mkdir -p /var/opt
mkdir -p /var/run
mkdir -p /var/spool
mkdir -p /var/lock/subsys
mkdir -p /var/netstate
mkdir -p /var/tmp
mkdir -p /var/db
mkdir -p /var/emulab/boot/tmcc
mkdir -p /var/emulab/db
mkdir -p /var/emulab/jails
mkdir -p /var/emulab/lock
mkdir -p /var/emulab/logs

# Remove dhcpcd temp files (XXX: shouldn't have to do this..)
rm -f /etc/dhcpc/dhcpcd-*
