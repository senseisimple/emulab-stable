#!/bin/sh
#
# This is the script exec'ed by vserver-init.
# We wait until the outside world has declared that things are ready
# and then we run all the normal (remaining) startup scripts.
#

#
# Now find out what the current and what the previous runlevel are.
# XXX passed in on command line
#
runlevel="$1"
previous="$2"
export runlevel previous

. /etc/init.d/functions
. /etc/emulab/paths.sh

# check a file to be a correct runlevel script
check_runlevel ()
{
	# Check if the file exists at all.
	[ -x "$1" ] || return 1
	is_ignored_file "$1" && return 1
	return 0
}

spinbubbaspin ()
{
    trap "exit 0" HUP

    while true; do
	sleep 300
    done
    exit 0
}

RETVAL=0

#
# Not "ready".  This indicates that we are likely just starting up.  At this
# point, the outside needs a pid for the inside in order to inject network
# interfaces.  However, our pid appears to us as 1, which is no help at all.
# So we temporarily create a process just so we can use its pid to identify
# our namespace.  Once the outside has done its dirty work, we kill the pid.
#
if [ ! -r $BOOTDIR/ready ]; then
    if [ -r $BOOTDIR/vserver.pid ]; then
	echo "vserver pidproc already exists!?"
    else
	spinbubbaspin &
	echo $! > $BOOTDIR/vserver.pid
    fi

    #
    # Wait for outside to declare us ready.
    # Knock off bubba when done.
    #
    while [ ! -r $BOOTDIR/ready ]; do
	echo "vserver waiting for ready signal..."
	sleep 2
    done

    if [ ! -r $BOOTDIR/vserver.pid ]; then
	# something went wrong outside, just exit   
	echo "vserver aborted"
	exit 1
    fi

    bubbapid=`cat $BOOTDIR/vserver.pid`
    rm -f $BOOTDIR/vserver.pid
    kill -HUP $bubbapid
fi

# First, run the KILL scripts.
for i in /etc/rc$runlevel.d/K* ; do
        check_runlevel "$i" || continue

        # Check if the subsystem is already up.
        subsys=${i#/etc/rc$runlevel.d/K??}
        [ -f /var/lock/subsys/$subsys -o -f /var/lock/subsys/$subsys.init ] \
                || continue

        # Bring the subsystem down.
        if LC_ALL=C egrep -q "^..*init.d/functions" $i ; then
                $i stop
        else
                action $"Stopping $subsys: " $i stop
        fi
done

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
