#!/bin/sh
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#

#
# Special shutdown script for testbed nodes
#

case "$1" in
    start)
	#
	# Nothing to do on node startup
	#
	;;
    stop)
	#
	# Inform the testbed that we're rebooting
	#
	echo "Informing the testbed we're rebooting"
	/etc/testbed/tmcc state REBOOTING
	;;
    *)
	echo "Uknown option $1 - should be 'start' or 'stop'"
	;;
esac
