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
# page. Its extremely helpful with jailed nodes, where rdpd is either
# running on another port, or on a private IP. Please see the Emulab FAQ
# for instructions on how to install this helper program. 
#
# Obviously, it helps to have an ssh agent running.
# 
sub usage()
{
    print(STDERR "rdp-mime-winxp.pl <control-file>\n");
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
	/^hostname:\s*([-\w\.]+)$/ && do {
	    $hostname = $1;
	    last SWITCH1;
	};
    }
}
close(CONFIG);

#
# Must have a hostip. Port is optional.
#
if (!defined($hostname)) {
    printf("Config file must specify a hostname\n");
    sleep(5);
    die("");
}

exec "mstsc /v:$hostname";
