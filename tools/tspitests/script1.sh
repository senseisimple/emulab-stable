#!/bin/sh

# EMULAB-COPYRIGHT
# Copyright (c) 2010 University of Utah and the Flux Group.
# All rights reserved.

MP=`which modprobe`
KILLALL=`which killall`
TCSD=`which tcsd`
DOQ=`which doquote`
TMCC=`which tmcc`
SLEEPTIME=1

# Error check later
/etc/testbed/rc/rc.tpmsetup

${MP} tpm_tis
${KILLALL} -9 tcsd; sleep ${SLEEPTIME}
${TCSD}

SSCRUFT=`${TMCC} quoteprep RELOADSETUP | ${DOQ} RELOADSETUP`
${TMCC} securestate ${SSCRUFT}

${KILLALL} -9 tcsd; sleep ${SLEEPTIME}
${TCSD}
${TMCC} -T imagekey > /tmp/secureloadinfo.out

