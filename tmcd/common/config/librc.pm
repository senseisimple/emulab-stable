#!/usr/bin/perl -wT
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2004 University of Utah and the Flux Group.
# All rights reserved.
#

#
# Tuny helper library for Emulab rc scripts.
#
package librc;
use Exporter;
@ISA    = "Exporter";
@EXPORT = qw(fatal warning scriptname logit);

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

#
# Log something to the console. The image will setup syslogd.conf so that
# local5.err goes to the console!
#
sub logit($$)
{
    my ($tag, $msg) = @_;
    
    system("logger -p local5.err -i -t $tag '$msg'");
}

#
# Fatal error. Display and die.
# 
sub fatal($)
{
    my($mesg) = $_[0];

    die("*** $0:\n".
	"    $mesg\n");
}

#
# Display warning.
# 
sub warning($)
{
    my($mesg) = $_[0];

    print("*** $0:\n".
	  "    WARNING: $mesg\n");
}

#
# Return scriptname with no path.
#
sub scriptname()
{
    my ($dirpath,$base) = ($PROGRAM_NAME =~ m#^(.*/)?(.*)#);

    return $base;
}

1;

