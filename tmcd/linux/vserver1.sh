#!/bin/sh
#
# This is the script run by chpid once it has cloned.
# We run all the normal startup scripts.
#

# Now find out what the current and what the previous runlevel are.
argv1="$1"
set `/sbin/runlevel`
runlevel=$2
previous=$1
export runlevel previous

. /etc/init.d/functions
. /etc/emulab/paths.sh

# check a file to be a correct runlevel script
check_runlevel ()
{
        # Do not redo the chpid!
        [ "$1" = "/etc/rc$runlevel.d/S00chpid" ] && return 1

	# Check if the file exists at all.
	[ -x "$1" ] || return 1
	is_ignored_file "$1" && return 1
	return 0
}

RETVAL=0

# Now run the START scripts.
for i in /etc/rc$runlevel.d/S* ; do
	check_runlevel "$i" || continue

	# Check if the subsystem is already up.
	subsys=${i#/etc/rc$runlevel.d/S??}
	[ -f /var/lock/subsys/$subsys -o -f /var/lock/subsys/$subsys.init ] \
		&& continue
		    
	update_boot_stage "$subsys"
	# Bring the subsystem up.
	if [ "$subsys" = "halt" -o "$subsys" = "reboot" ]; then
		export LC_ALL=C
		exec $i start
	fi
	if LC_ALL=C egrep -q "^..*init.d/functions" $i \
			|| [ "$subsys" = "single" -o "$subsys" = "local" ]; then
		$i start
	else
		action $"Starting $subsys: " $i start
	fi
done

exit $RETVAL
