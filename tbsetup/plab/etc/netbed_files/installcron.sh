#!/bin/sh

CRONTAB=/usr/bin/crontab
CRONLINE="@reboot cd $HOME; ./netbed_files/sbin/thttpd.restart"

#
# For now, just install the new crontab - we don't really have a reason to 
# put anything else into it
#
echo $CRONLINE | $CRONTAB -
