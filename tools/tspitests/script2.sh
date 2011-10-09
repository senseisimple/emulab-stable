#!/bin/sh
#
# EMULAB-COPYRIGHT
# Copyright (c) 2010-2011 University of Utah and the Flux Group.
# All rights reserved.
#

KILLALL=`which killall`
TCSD=`which tcsd`
TPMS=`which tpm-signoff`
DOQ=`which doquote`
TMCC=`which tmcc`
SLEEPTIME=1

REBOOTPCR=15

# Error check later

echo "Setting sign-off PCR ..."
${KILLALL} -9 tcsd; sleep ${SLEEPTIME}
${TCSD}
${TPMS} ${REBOOTPCR}

${KILLALL} -9 tcsd; sleep ${SLEEPTIME}
${TCSD}

echo "Requesting info for TPMSIGNOFF quote ..."
QINFO=`${TMCC} quoteprep TPMSIGNOFF`
if [ -z "$QINFO" ]; then
    echo "*** could not get TPMSIGNOFF quote info"
    exit 1
fi

echo "Preparing TPMSIGNOFF quote ..."
SSCRUFT=`echo $QINFO | ${DOQ} TPMSIGNOFF`
if [ -z "$SSCRUFT" ]; then
    echo "*** could not produce TPMSIGNOFF quote"
    exit 1
fi

echo "Sending TPMSIGNOFF quote ..."
RC=`${TMCC} securestate ${SSCRUFT}`
if [ $? -ne 0 -o "$RC" = "FAILED" ]; then
    echo "*** could not transition to TPMSIGNOFF"
    exit 1
fi

exit 0
