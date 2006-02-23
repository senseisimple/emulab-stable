#!/bin/sh

# PROVIDE: mysql-client
# REQUIRE: NETWORKING SERVERS ldconfig
# BEFORE: mysql apache
# KEYWORD: shutdown

case "$1" in
	start|faststart)
		/sbin/ldconfig -m /usr/local/lib/mysql
		;;
	stop)
		;;
	*)
		echo ""
		echo "Usage: `basename $0` { start | stop }"
		echo ""
		exit 64
		;;
esac
