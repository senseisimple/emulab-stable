#!/bin/sh
#
# Emulab Initscript. Completely ripped from the stork initscript!
#
echo "Starting Emulab initscript"

# check for root user
if [ $UID -ne "0" ]
then
   echo "You must run this program with root permissions..."
   exit 1
fi

# log everything for diagnosis
exec >> /tmp/emulab_initscript.log
exec 2>&1

# mark when the stage1 initscript started running
date

# filename for the extract stage2 initscript
STAGE2_FN=`mktemp -t planet-initscript.XXXXXXXXXX`

# URL for the stage2 initscript
STAGE2_URL="http://www.emulab.net/downloads/planetlab-initscript.pl"

DELAY=1
RETRY=1

while [ $RETRY -eq 1 ]; do
   # break out of loop by default
   RETRY=0

   # download the stage2 initscript
   wget $STAGE2_URL -O $STAGE2_FN
   if [ ! -f $STAGE2_FN -o $? -ne 0 ]
   then
      echo "Failed to download $STAGE2_URL"
      RETRY=1
   fi

   # if something went wrong, then retry. This covers connection errors as
   # well as problems with the repository itself. If the repository is
   # offline, then the script will keep retrying until it comes online.
   if [ $RETRY -eq 1 ]; then
      echo "Delaying $DELAY seconds"

      sleep $DELAY

      # exponential backoff, up to 1 hour between retrievals
      DELAY=`expr $DELAY \* 2`
      if [ $DELAY -gt 3600 ]; then
          DELAY=3600
      fi
   fi

done

echo "running stage2 initscript"
perl $STAGE2_FN
