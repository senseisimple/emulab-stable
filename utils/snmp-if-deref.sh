#!/bin/sh
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#

SNMPGET="/usr/local/bin/snmpget -m CISCO-STACK-MIB:CISCO-PAGP-MIB -Ovq"
community="public"

if [ $# -lt 4 ]
then
    echo "usage: $0 <address> <module> <port> <ifEntry-attribute> [trunk]"
    exit 1
fi

addr=$1
module=$2
port=$3
attr=$4
trunk=$5
    

index=`$SNMPGET $addr $community portIfIndex.$module.$port 2> /dev/null`
if [ $? -eq 0 -a "x$index" != "x" ]
then
    if [ "x$trunk" = "xtrunk" ]
    then
        index=`$SNMPGET $addr $community pagpGroupIfIndex.$index 2> /dev/null`
    fi

    retval=`$SNMPGET $addr $community ifEntry.$attr.$index 2> /dev/null`
    if [ $? -eq 0 -a "x$retval" != "x" ]
    then
	echo $retval
	exit 0
    else
	echo "Error getting ifEntry attribute value: $attr" 1>&2
	exit 1
    fi
else
    echo "Error looking up index for module: $module, port: $port" 1>&2
    exit 1
fi
