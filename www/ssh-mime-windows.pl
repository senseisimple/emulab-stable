#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#
use English;
use Getopt::Std;

#
# This is a helper program for your web browser. It allows you to ssh
# to an experimental node by clicking on a menu option in the shownode
# page. Its extremely helpful with jailed nodes, where sshd is either
# running on another port, or on a private IP. Please see the Emulab FAQ
# for instructions on how to install this helper program. 
#
# Obviously, it helps to have an ssh agent running.
# 
sub usage()
{
    print(STDERR "ssh-mime.pl <control-file>\n");
}
my $optlist = "";
my $config;

my $puttypath = "C:\/putty.exe";
my $tmpfile = "C:\/temp\/sshcmd";
my $logfile = "C:\/temp\/ssh-mime.log";

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
# Log all messages to a file
#
open STDOUT, ">$logfile" or die "Unable to log to $logfile: $!\n";
open STDERR, '>&STDOUT' or die "Can't dup stdout: $!";

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
	    $login = "-l $1";
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

#
# Exec an ssh.
#
if (!defined($gateway)) {
    exec "$puttypath -ssh $login $port $hostname";
}
else {
    open TF, ">$tmpfile" or die "Unable to open $tmpfile\n";
    print TF "ssh -tt -o StrictHostKeyChecking=no $port $hostname";
    close TF;
    exec "$puttypath -A -ssh -t $login -m $tmpfile $gateway";
}
