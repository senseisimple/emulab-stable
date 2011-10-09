#!/bin/sh

# EMULAB-COPYRIGHT
# Copyright (c) 2010 University of Utah and the Flux Group.
# All rights reserved.

KILLALL=`which killall`
TCSD=`which tcsd`
TPMS=`which tpm-signoff`
DOQ=`which doquote`
TMCC=`which tmcc`
SLEEPTIME=1

REBOOTPCR=15

# Error check later

${KILLALL} -9 tcsd; sleep ${SLEEPTIME}
${TCSD}
${TPMS} ${REBOOTPCR}

${KILLALL} -9 tcsd; sleep ${SLEEPTIME}
${TCSD}

SSCRUFT=`${TMCC} quoteprep TPMSIGNOFF | ${DOQ} TPMSIGNOFF`
${TMCC} securestate ${SSCRUFT}

