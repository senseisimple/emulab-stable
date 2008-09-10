#!/usr/bin/perl -wT
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2004 University of Utah and the Flux Group.
# All rights reserved.
#

#
# This is a stub library to provide a few things that libtestbed on
# boss provides.
#
package libtestbed;
use Exporter;
@ISA    = "Exporter";
@EXPORT = qw( TB_BOSSNODE TB_EVENTSERVER );

# Must come after package declaration!
use English;

#
# Turn off line buffering on output
#
$| = 1;

# Load up the paths. Done like this in case init code is needed.
BEGIN
{
    if (! -e "/etc/emulab/paths.pm") {
	die("Yikes! Could not require /etc/emulab/paths.pm!\n");
    }
    require "/etc/emulab/paths.pm";
    import emulabpaths;
}

# Need this.
use libtmcc;

#
# Return name of the bossnode.
#
sub TB_BOSSNODE()
{
    return tmccbossname();
}

#
# Return name of the event server.
#
sub TB_EVENTSERVER()
{
    # duplicate behavior of tmcc bossinfo function
    my @searchdirs = ( "/etc/testbed","/etc/emulab","/etc/rc.d/testbed",
		       "/usr/local/etc/testbed","/usr/local/etc/emulab" );
    my $bossnode = TB_BOSSNODE();
    my $eventserver = '';

    foreach my $d (@searchdirs) {
	if (-e "$d/eventserver" && !(-z "$d/eventserver")) {
	    $eventserver = `cat $d/eventserver`;
	    last;
	}
    }
    if ($eventserver eq '') {
	my @ds = split(/\./,$bossnode,2);
	if (scalar(@ds) == 2) {
	    # XXX event-server hardcode
	    $eventserver = "event-server.$ds[1]";
	}
    }
    if ($eventserver eq '') {
	$eventserver = "event-server";
    }

    return $eventserver;
}

1;

