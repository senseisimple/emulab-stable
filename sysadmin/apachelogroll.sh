#! /bin/sh

PIDFILE=/var/run/apache.pid
LOGDIR=/usr/testbed/log
DESTDIR=/z/testbed/logs/apache
APACHECTL=/usr/local/sbin/apachectl
LOGS='apache_access_log apache_error_log apache_ssl_engine_log 
apache_ssl_request_log'
SIZELIMIT=5000  # about 5 MB
DATE=`date '+%Y-%m-%d'`
MOVED=0
MAXTRIES=10

cd $LOGDIR
   
for CURLOG in $LOGS
do
    if [ -f $CURLOG -a ! -e ${CURLOG}.${DATE} ]
    then
        LOGSIZE=`ls -sk $CURLOG | awk '{ print $1 }'`
        if [ $LOGSIZE -gt $SIZELIMIT ]
        then
            mv $CURLOG ${CURLOG}.${DATE}
            MOVED=1
        fi
    fi
done

if [ $MOVED -eq 1 ]
then
    $APACHECTL graceful
    if [ $? -ne 0 ]
    then
        echo "Apache restart failed"
        exit 1
    fi

    sleep 60

    for CURLOG in `ls *.${DATE}`
    do
        while fstat $CURLOG | grep -q -v "USER"
        do
          sleep 60
          COUNT=`expr $COUNT + 1`
          if [ $COUNT -gt $MAXTRIES ]
          then
              echo "Tired of waiting for file to become free"
              exit 1
          fi
        done

        gzip -9 $CURLOG && cp $CURLOG $DESTDIR && rm $CURLOG
        if [ $? -ne 0 ]
        then
          echo "Error trying to zip and move log."
          exit 1
        fi
      done
fi

exit 0
