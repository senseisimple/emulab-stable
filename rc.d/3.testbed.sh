#!/bin/sh
#
# Utah Network Testbed local startup
#
if [ -x /usr/testbed/sbin/bootinfo.restart  ]; then
        echo -n " bootinfo"
        /usr/testbed/sbin/bootinfo.restart
fi

if [ -x /usr/testbed/sbin/tmcd.restart  ]; then
        echo -n " tmcd"
        /usr/testbed/sbin/tmcd.restart
fi

if [ -x /usr/testbed/sbin/proxydhcp.restart  ]; then
	echo -n " proxydhcp"
	/usr/testbed/sbin/proxydhcp.restart
fi

if [ -x /usr/site/etc/capture.rc -a -d /var/log/tiplogs ]; then
	echo -n " capture"
	/usr/site/etc/capture.rc
fi

if [ -x /usr/testbed/sbin/reload_daemon  ]; then
        echo -n " reloadd"
        /usr/testbed/sbin/reload_daemon
fi

