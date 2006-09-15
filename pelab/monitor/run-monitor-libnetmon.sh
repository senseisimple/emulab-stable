#!/bin/sh
#
# Script to run the monitor, collecting data from libnetmon
#

#
# Let common-env know what role we're playing
#
export HOST_ROLE="monitor"

#
# Grab common environment variables
#
. `dirname $0`/../common-env.sh

REAL_PLAB=0
if [ $# != 0 ]; then
    if [ $# -ge 3 ]; then
      echo "Usage: $0 [stub-ip]"
      exit 1;
    fi
    if [ $1 = "-p" ]; then
        REAL_PLAB=1
        SIP=$2
    else
        SIP=$1
    fi

fi

if ! [ -x "$NETMON_DIR/$NETMOND" ]; then
    gmake -C $NETMON_DIR $NETMOND
fi

if ! [ -x "$NETMON_DIR/$NETMOND" ]; then
    echo "$NETMON_DIR/$NETMOND missing - run 'gmake' in $NETMOND_DIR to build it"
    exit 1;
fi

if [ $REAL_PLAB -eq 1 ]; then
    echo "Generating IP mapping file for the real PlanetLab into $IPMAP";
    $PERL ${MONITOR_DIR}/$GENIPMAP -p > $IPMAP
else
    echo "Generating IP mapping file for fake PlanetLab into $IPMAP";
    $PERL ${MONITOR_DIR}/$GENIPMAP > $IPMAP
fi

INITARG=""
if [ -r "/proj/$PROJECT/exp/$EXPERIMENT/tmp/initial-conditions.txt" ]; then
    echo "Copy over initial conditions file for the real PlanetLab nodes";
    cp -p /proj/$PROJECT/exp/$EXPERIMENT/tmp/initial-conditions.txt $INITCOND
    INITARG="--initial=$INITCOND"
fi


#echo "Starting up monitor for $PROJECT/$EXPERIMENT $PELAB_IP $SIP";
echo "Starting up monitor with options --mapping=$IPMAP --experiment=$PROJECT/$EXPERIMENT --ip=$PELAB_IP --initial=$MONITOR_DIR/initial.txt";
exec $NETMON_DIR/$NETMOND -v 2 -f 262144 | tee $LOGDIR/libnetmon.out | $PYTHON $MONITOR_DIR/$MONITOR --mapping=$IPMAP --experiment=$PROJECT/$EXPERIMENT --ip=$PELAB_IP $INITARG
#exec $NETMON_DIR/$NETMOND -v 2 | $PYTHON $MONITOR_DIR/$MONITOR ip-mapping.txt $PROJECT/$EXPERIMENT $PELAB_IP $SIP
