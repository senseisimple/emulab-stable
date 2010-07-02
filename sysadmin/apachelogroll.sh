#! /bin/sh
#
# EMULAB-COPYRIGHT
# Copyright (c) 2001-2006 University of Utah and the Flux Group.
# All rights reserved.
#

# CONSTANTS INIT
PIDFILE=/var/run/apache.pid
LOGDIR=/usr/testbed/log
DESTDIR=/z/testbed/logs/apache
APACHECTL=/usr/local/sbin/apachectl
LOGS='apache_access_log apache_error_log apache_ssl_engine_log apache_ssl_request_log'
SIZELIMIT=5000  # about 5 MB
DATE=`date '+%Y-%m-%d'`
MAXTRIES=10

# GLOBAL VARIABLES INIT
MOVED=0

cd $LOGDIR

#
# CHECK THE LOG FILES
#
# For each apache log, check its size to see if it has exceeded the size
# limit above.  If so, move it to a temp filename for archival
#   
for CURLOG in $LOGS
do
    if [ -f $CURLOG -a ! -e ${CURLOG}.${DATE} ]
    then
        LOGSIZE=`ls -sk $CURLOG | awk '{ print $1 }'`
        if [ $LOGSIZE -gt $SIZELIMIT ]
        then
            mv $CURLOG ${CURLOG}.alr.${DATE}
            echo "Rolling $CURLOG at $LOGSIZE kbytes"
            MOVED=1
        fi
    fi
done

#
# ARCHIVE THE LOG FILES
#
# OK, if any files have been moved, they need to be archived.  First, we need
# to restart apache so that it will reopen its log files under their original
# names (rather than using the moved ones, which it still has open initially).
# Don't try to archive the open, moved log files until apache closes them.
#
# Don't restart apache if it isn't running.  Do always archive the logs if
# necessary though.
#
if [ $MOVED -eq 1 ]
then
    # Should be a proc entry if the process is running!
    # Note that a graceful restart waits for all clients to disconnect.
    if [ -e /proc/`cat $PIDFILE` ]
    then
        $APACHECTL graceful
        if [ $? -ne 0 ]
        then
            echo "Apache restart failed"
            exit 1
        fi
    fi

    # wait a bit for apache to restart
    sleep 60

    # Now, lets try to archive off those logs..
    for CURLOG in `ls *.alr.*`
    do
        # This while loop makes sure nothing still has the moved logfile open.
        COUNT=$MAXTRIES
        while fstat $CURLOG | grep -q -v "USER"
        do
          sleep 60

          if [ $COUNT -eq 0 ]
          then
              # Even if we give up, we'll catch this log next time the
              # script is run (*.alr.*)
              echo "Tired of waiting for $CURLOG to become free .. skipping."
              continue 2
          fi
          COUNT=$(($COUNT-1))
        done

        # deflate, and move (safely).
        TMPLOG=`echo $CURLOG | sed -e 's/\.alr//'`
        test ! -e $TMPLOG && \
        mv $CURLOG $TMPLOG && \
        gzip -9 $TMPLOG && \
        cp $TMPLOG.gz $DESTDIR && \
        rm $TMPLOG.gz

        if [ $? -ne 0 ]
        then
          echo "Error trying to zip and move $CURLOG."
          exit 1
        fi
      done
fi

exit 0
