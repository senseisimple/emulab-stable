#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#
use English;
use Getopt::Std;

#
# This is a helper program for your web browser. It allows you to rdp
# to an experimental node by clicking on a menu option in the shownode
# page. Its extremely helpful with jailed nodes, where sshd is either
# running on another port, or on a private IP. Please see the Emulab FAQ
# for instructions on how to install this helper program. 
#
# Obviously, it helps to have an ssh agent running.
# 
sub usage()
{
    print(STDERR "rdp-mime.pl <control-file>\n");
}
my $optlist = "";
my $config;

# Locals
my $hostname;
my $gateway;
my $port    = "";
my $login   = "";

#
# Turn off line buffering on output
#
$| = 1;

#
# Parse command arguments. Once we return from getopts, all that should be
# left are the required arguments.
#
%options = ();
if (! getopts($optlist, \%options)) {
    usage();
}
if (@ARGV != 1) {
    usage();
}
$config = $ARGV[0];

#
# Open up the config file. It tells us what to do.
#
open(CONFIG, "< $config")
    or die("Could not open config file $config: $!\n");

while (<CONFIG>) {
    chomp();
    SWITCH1: {
	/^port:\s*(\d+)$/ && do {
	    $port = "-p $1";
	    last SWITCH1;
	};
	/^hostname:\s*([-\w\.]+)$/ && do {
	    $hostname = $1;
	    last SWITCH1;
	};
	/^gateway:\s*([-\w\.]+)$/ && do {
	    $gateway = $1;
	    last SWITCH1;
	};
	/^login:\s*([-\w]+)$/ && do {
	    $login = "-u $1";
	    last SWITCH1;
	};
    }
}
close(CONFIG);

#
# Must have a hostip. Port is optional.
#
if (!defined($hostname)) {
    die("Config file must specify a hostname\n");
}

# Run rdesktop in its own directory so it finds the keymaps subdirectory.
#
# You can specify any display resolution you want; it doesn't have to be
# one of the "normal" ones.  And you can switch back and forth by just starting
# a new rdesktop and "grabbing" the rlogin session away from the previous one.
# 
# But once an rdesktop is started up, its display resolution is fixed.  If you make it
# smaller than the previous one, it will push your windows around to fit.
#
my $rd = 'cd ~fish/misc/rdesktop/rdesktop-1.3.1; ./rdesktop -K -g 1280x1024';
if (!defined($gateway)) {
    exec "$rd $login $hostname &"; ### -p $pswd
}
else {
    die("No proxying yet.\n");
}
