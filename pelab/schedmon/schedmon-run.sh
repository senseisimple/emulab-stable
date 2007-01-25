#/bin/sh

SCHEDMON="/local/schedmon/schedmon -s 10000 -n 100 -t -l 59 -b"
ACTIVELOG=/tmp/schedmon.log

loop=1
function hup_h () {
  loop=1
}
function term_h () {
  loop=0
}

# just to be safe
killall -w -SIGKILL schedmon > /dev/null 2>&1 

while [ $loop -ne 0 ]
do
  loop=0
  trap hup_h  SIGHUP
  trap term_h SIGTERM

  # run until it dies via a signal
  $SCHEDMON > $ACTIVELOG

  trap '' SIGTERM # to make sure the log gets copied

  ID=`date -u +%Y%m%d%H%M%S`

  # now move and compress the log
  cat $ACTIVELOG | gzip -c > /local/logs/schedmon-$ID.log.gz
  rm $ACTIVELOG
done

exit 0


