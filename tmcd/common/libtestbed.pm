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
@EXPORT = qw( TB_BOSSNODE );

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

1;

