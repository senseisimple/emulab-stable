#!/bin/csh

cd /usr/local/etc/emulab

setenv OSTYPE `uname -s`

#
# Insert stuff here to fix things up
#
if ($OSTYPE == "FreeBSD") then
	echo "Updating Freebsd";
else if ($OSTYPE == "Linux") then
	echo "Updating Linux";
else
	echo "Unsupported OS: $OSTYPE";
	exit 1;
endif

#
# Then restart the watchdog.
#
if ($OSTYPE == "FreeBSD") then
	/usr/local/etc/rc.d/emulab.sh stop
	sleep 1
	/usr/local/etc/rc.d/emulab.sh start
else
	/etc/init.d/emulab stop
	sleep 1
	/etc/init.d/emulab start
endif
