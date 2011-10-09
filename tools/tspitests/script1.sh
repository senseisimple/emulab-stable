#!/bin/sh
#
# EMULAB-COPYRIGHT
# Copyright (c) 2010-2011 University of Utah and the Flux Group.
# All rights reserved.
#

MP=`which modprobe`
KILLALL=`which killall`
TCSD=`which tcsd`
DOQ=`which doquote`
TMCC=`which tmcc`
SLEEPTIME=1

# Error check later
echo "Doing TPM setup ..."
/etc/testbed/rc/rc.tpmsetup

${MP} tpm_tis
${KILLALL} -9 tcsd; sleep ${SLEEPTIME}
${TCSD}

echo "Requesting info for RELOADSETUP quote ..."
QINFO=`${TMCC} quoteprep RELOADSETUP`
if [ -z "$QINFO" ]; then
    echo "*** could not get RELOADSETUP quote info"
    exit 1
fi

echo "Preparing RELOADSETUP quote ..."
SSCRUFT=`echo $QINFO | ${DOQ} RELOADSETUP`
if [ -z "$SSCRUFT" ]; then
    echo "*** could not produce RELOADSETUP quote"
    exit 1
fi

echo "Sending RELOADSETUP quote ..."
RC=`${TMCC} securestate ${SSCRUFT}`
if [ $? -ne 0 -o "$RC" = "FAILED" ]; then
    echo "*** could not transition to RELOADSETUP"
    exit 1
fi

${KILLALL} -9 tcsd; sleep ${SLEEPTIME}
${TCSD}

exit 0
