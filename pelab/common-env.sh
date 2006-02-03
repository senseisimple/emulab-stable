#!/bin/sh
#
# Simple shell script to get some environment variables useful for pelab
# shell scripts
#

#
# Node/experiment info
#
export NICKNAME=`cat /var/emulab/boot/nickname`;
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
