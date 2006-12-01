#!/bin/sh
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#

#
# Simple shell script to get some environment variables useful for pelab
# shell scripts
#

#
# 'Header guard'
#
if [ "$COMMON_ENV_LOADED" != "yes" ]; then
COMMON_ENV_LOADED="yes"

#
# Find out the OS we're running on
#
UNAME=`uname -s`

#
# Yow.  The BSD shell requires setting trapsasync (-T) to allow us to
# receive signals when in a wait.
#
if [ "$UNAME" = "FreeBSD" ]; then
    set -o trapsasync
    SH="/bin/sh -T"
else
    SH=/bin/sh
fi

#
# Different binary directories for FreeBSD/Linux
#
if [ "$UNAME" = "Linux" ]; then
    BIN_PATH="/usr/bin"
elif [ "$UNAME" = "FreeBSD" ]; then
    BIN_PATH="/usr/local/bin"
else
    echo "Unable to determine operating system"
    exit -1
fi

#
# Locations of some Emulab-specific files
#
EMUVAR="/var/emulab"
EMUBOOT="$EMUVAR/boot"
NICKFILE="$EMUBOOT/nickname"
RCPLAB="$EMUBOOT/rc.plab"
HOSTSFILE="/etc/hosts"
IFCONFIG="/sbin/ifconfig"
PERL="/usr/bin/perl"
PYTHON="${BIN_PATH}/python"
SUDO="${BIN_PATH}/sudo"
MKDIR="/bin/mkdir"
CHMOD="/bin/chmod"
SU="/bin/su"

SYNC="/usr/local/etc/emulab/emulab-sync"
SYNCTIMO=120

if [ "$UNAME" = "Linux" ]; then
    GREP="/bin/grep"
elif [ "$UNAME" = "FreeBSD" ]; then
    GREP="/usr/bin/grep"
else
    echo "Unable to determine operating system"
    exit -1
fi

#
# Some 'constants' by convention. 
#
export PELAB_LAN=elabc
export EPLAB_LAN=plabc

#
# Name of the barrier for indicating all stubs are ready
#
export STUB_BARRIER=stubsready

#
# Node/experiment info
#
export NICKNAME=`cat $NICKFILE`
export HOSTNAME=`echo $NICKNAME | cut -d. -f1`;
export EXPERIMENT=`echo $NICKNAME | cut -d. -f2`;
export PROJECT=`echo $NICKNAME | cut -d. -f3`;


#
# Important directories
#
SCRIPT_LOCATION=`dirname $0`
export BASE="${SCRIPT_LOCATION}/../";
export STUB_DIR="${BASE}/stub/";
export MAGENT_DIR="${BASE}/magent/";
export NETMON_DIR="${BASE}/libnetmon/";
export MONITOR_DIR="${BASE}/monitor/";
export DBMONITOR_DIR="${BASE}/dbmonitor/";
export IPERFD_DIR="${BASE}/iperfd/";
export TMPDIR="/var/tmp/";
export LOGDIR="/local/logs/"

#
# Temproary files we use.
# We put them in /local/logs so they become part of the fossil record.
#
export IPMAP="/local/logs/ip-mapping.txt"
export INITCOND="/local/logs/initial-conditions.txt"

#
# Important scripts/libraries
#
export NETMOND="netmond"
export STUBD="stubd"
export MAGENT="magent"
export MONITOR="monitor.py"
export DBMONITOR="dbmonitor.pl"
export IPERFD="iperfd"
export GENIPMAP="gen-ip-mapping.pl"
export GENINITCOND="init-elabnodes.pl"
export NETMON_LIB="libnetmon.so"

#
# Where are we running?
#
if [ -e "$RCPLAB" ]; then
    export RUNNING_ON="plab"
    export ON_PLAB="yes"
    export ON_ELAB=""
else
    export RUNNING_ON="elab"
    export ON_PLAB=""
    export ON_ELAB="yes"
fi

#
# Find out if we're in a vserver - can we see the init process?
#
ps --pid 1 > /dev/null
if [ $? == 1 ]; then
    export IN_VSERVER="yes"
else
    export IN_VSERVER=""
fi

#
# Are we the "master" (sync server) node?
#

if `$SYNC -m`; then
    export ON_MASTER="yes"
else
    export ON_MASTER=""
fi

#
# Make a handy variable for running as root (ie. invoke sudo if necessary)
#
if [ "$EUID" != "0" ]; then
    export AS_ROOT="$SUDO"
else
    export AS_ROOT=""
fi

#
# How big is this experiment? Counts the number of planetlab nodes
#
export PEER_PAIRS=`$GREP -E -c 'elab-.*-elabc ' /etc/hosts`
export PEERS=`expr $PEER_PAIRS \* 2`

#
# Make the logdir if it doesn't exist
#
if [ ! -d $LOGDIR ]; then
    $AS_ROOT $MKDIR -p $LOGDIR
    $AS_ROOT $CHMOD 777 $LOGDIR
fi

#
# IP addresses and interface names
#
ifacename() {
    LINKIP=$1
    IFACENAME=`$IFCONFIG -a | $PERL -e "while(<>) { if (/^(eth\d+)/) { \\\$if = \\\$1; } if (/$LINKIP/) { print \"\\\$if\n\"; exit 0; }} exit 1;"`

    echo $IFACENAME
}

lookup_ip_host()
{
    HOST=$1
    LINK=$2
    IPADDR=`grep "${HOST}-${LINK}[ ]" $HOSTSFILE | cut -f1`
    echo $IPADDR
}

get_iface_addr() {
    IFACE=$1
    IFACEADDR=`$IFCONFIG -a | $PERL -e "while(<>) { if (/^$IFACE/) { \\\$found = 1; } if (\\\$found && /addr:(\d+\.\d+\.\d+\.\d+)/) { print \"\\\$1\n\"; exit 0; }} exit 1;"`
    echo $IFACEADDR
}

#
# $HOST_ROLE should be set by the calling script
#
if [ "$HOST_ROLE" = "monitor" ]; then
    export PELAB_IP=`lookup_ip_host $HOSTNAME $PELAB_LAN`
    export PELAB_IFACE=`ifacename $PELAB_IP`
elif [ "$HOST_ROLE" = "stub" ]; then
    if [ "$ON_ELAB" = "yes" ]; then
        export PLAB_IP=`lookup_ip_host $HOSTNAME $EPLAB_LAN`
        #
        # We could still be in a vserver
        #
        if [ "$IN_VSERVER" = "yes" ]; then
            export PLAB_IFACE="vnet"
        else
            export PLAB_IFACE=`ifacename $PLAB_IP`
        fi
    else
        # On real planetlab
        export PLAB_IFACE="vnet"
        export PLAB_IP=`get_iface_addr $PLAB_IFACE`
    fi
fi

#
# Function for waiting on a barrier sync
# I'd rather automatically determine the number of peer pairs, but that
# looks too hard for now.
#
barrier_wait()
{
    BARRIER=$1

    if [ $ON_MASTER ]; then
        WAITERS=`expr $PEERS - 1`
        echo "Waiting up to $SYNCTIMO seconds for $WAITERS clients"
        sync_timeout $SYNCTIMO $SYNC -n $BARRIER -i $WAITERS
	_rval=$?
    else
	#
	# XXX on planetlab, lookup of the syncserver name can
	# fail transiently.  Try a couple more times, waiting in between.
	#
	for i in 1 2 3; do
	    $SYNC -n $BARRIER 
	    _rval=$?
	    if [ $_rval -ne 68 ]; then
	        break
	    fi
	    echo "sync on $BARRIER failed, trying again..."
	    sleep 10
	done
    fi

    return $_rval
}

#
# Log the stdout and stderr of the given process to the logdir
# Runs the program in the background and returns its pid
#
log_output_background()
{
    PROGNAME=$1
    CMD=$2
    $CMD 1> ${LOGDIR}/${PROGNAME}.stdout 2> ${LOGDIR}/${PROGNAME}.stderr &
    echo $!
}

sync_watchdog()
{
    TIMO=$1
    SYNCDPID=$2

    sleep $TIMO
    echo '*** HUPing syncd'
    $AS_ROOT kill -HUP $SYNCDPID
}

#
# If $SYNC command doesn't return within the indicated timeout period,
# HUP the syncserver to force everyone out of a barrier.
#
sync_timeout()
{
    TIMO=$1
    shift
    CMDSTR=$*

    if [ -r /var/run/syncd.pid ]; then
        SYNCDPID=`cat /var/run/syncd.pid`
    else
        SYNCDPID=""
    fi

    # fire off the command
    $CMDSTR & CMDPID=$!

    # and a watchdog
    if [ -n "$SYNCDPID" ]; then
        sync_watchdog $TIMO $SYNCDPID & DOGPID=$!
    fi

    # wait for the command to finish or be terminated
    wait $CMDPID
    RVAL=$?

    # nuke the watchdog
    if [ -n "$SYNCDPID" ]; then
        kill -9 $DOGPID >/dev/null 2>&1
    fi

    # and return the result
    return $RVAL
}

#
# Become the 'vuser'
#

fi # End of header guard
