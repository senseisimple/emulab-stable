#!/bin/sh
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
# Locations of some Emulab-specific files
#
EMUVAR="/var/emulab"
EMUBOOT="$EMUVAR/boot"
NICKFILE="$EMUBOOT/nickname"
RCPLAB="$EMUBOOT/rc.plab"
HOSTSFILE="/etc/hosts"
IFCONFIG="/sbin/ifconfig"
PERL="/usr/bin/perl"
PYTHON="/usr/bin/python"
SUDO="/usr/bin/sudo"

#
# Some 'constants' by convention. 
#
export PELAB_LAN=elabc
export EPLAB_LAN=planetc

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
export NETMON_DIR="${BASE}/libnetmon/";
export MONITOR_DIR="${BASE}/monitor/";
export TMPDIR="/var/tmp/";

#
# Important scrips/libraries
#
export NETMOND="netmond"
export STUBD="stubd"
export MONITOR="monitor.py"
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
# IP addresses and interface names
#
ifacename() {
    IPADDR=$1
    LINKIP=`lookup_ip_host $1`
    IFACENAME=`$IFCONFIG -a | $PERL -e "while(<>) { if (/^(eth\d+)/) { \\\$if = \\\$1; } if (/$LINKIP/) { print \"\\\$if\n\"; exit 0; }} exit 1;"`

    echo $IFACENAME
}

lookup_ip_host()
{
    HOST=$1
    LINK=$2
    IPADDR=`grep "$HOST-$LINK" $HOSTSFILE | cut -f1`
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
if [ "$HOST_ROLE" == "monitor" ]; then
    export PELAB_IP=`lookup_ip_host $HOSTNAME $PELAB_LAN`
    export PELAB_IFACE=`ifacename $PELAB_IP`
elif [ "$HOST_ROLE" == "stub" ]; then
    if [ "$ON_ELAB" == "yes" ]; then
        export PLAB_IP=`lookup_ip_host $HOSTNAME $EPLAB_LAN`
        export PLAB_IFACE=`ifacename $PLAB_IP`
    else
        # On real planetlab
        export PLAB_IFACE="vnet"
        export PLAB_IP=`get_iface_addr $PLAB_IFACE`
    fi
fi

#
# Make a handy variable for running as root (ie. invoke sudo if necessary)
#
if [ "$EUID" != "0" ]; then
    export AS_ROOT="$SUDO"
else
    export AS_ROOT=""
fi

fi # End of header guard
