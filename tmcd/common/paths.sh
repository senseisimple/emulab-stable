#!/bin/sh
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2004 University of Utah and the Flux Group.
# All rights reserved.
#

#
# This path stuff will go away when the world is consistent. Until then
# we need to be able to upgrade individual scripts to the various setups.
# Of course, the old setups need /etc/emulab/paths.{sh,pm} installed
# if any new scripts are installed too. I know, what a mess.
#
LOGDIR=/var/tmp
LOCKDIR=/var/tmp

if [ -d /usr/local/etc/emulab ]; then
	BINDIR=/usr/local/etc/emulab
	if [ -e /etc/emulab/client.pem ]; then
	    ETCDIR=/etc/emulab
	else
	    ETCDIR=/usr/local/etc/emulab
	fi
	VARDIR=/var/emulab
	BOOTDIR=/var/emulab/boot
	LOGDIR=/var/emulab/logs
	LOCKDIR=/var/emulab/lock
	DBDIR=/var/emulab/db
elif [ -d /etc/testbed ]; then
	ETCDIR=/etc/testbed
	BINDIR=/etc/testbed
	VARDIR=/etc/testbed
	BOOTDIR=/etc/testbed
	LOGDIR=/tmp
	LOCKDIR=/tmp
	DBDIR=/etc/testbed
elif [ -d /etc/rc.d/testbed ]; then
	ETCDIR=/etc/rc.d/testbed
	BINDIR=/etc/rc.d/testbed
	VARDIR=/etc/rc.d/testbed
	BOOTDIR=/etc/rc.d/testbed
	DBDIR=/etc/rc.d/testbed
else
        echo "$0: Cannot find proper emulab paths!"
	exit 1
fi

export ETCDIR
export BINDIR
export VARDIR
export BOOTDIR
export LOGDIR
export DBDIR
export LOCKDIR
PATH=$BINDIR:/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin:\
/usr/site/bin:/usr/site/sbin
