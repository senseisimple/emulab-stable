#!/bin/sh

if [ -x /usr/local/sbin/cvsupd ];
then
	/usr/local/sbin/cvsupd -l /var/log/cvsup.log -C 100 -b /usr/testbed/sup
fi
