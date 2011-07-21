#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2009 University of Utah and the Flux Group.
# All rights reserved.
#
use English;
use Getopt::Std;

#
# Perl wrapper to TMCC. Everything is done in the library.
#
# The library parses the options.
sub usage()
{
    print STDERR "usage: tmcc [options] <command> [arg1 ...]\n";
    print STDERR " -d		Turn on debugging\n";
    print STDERR " -b		Bypass the tmcc cache and goto to server\n";
    print STDERR " -s server	Specify a tmcd server to connect to\n";
    print STDERR " -p portnum	Specify a port number to connect to\n";
    print STDERR " -v versnum	Specify a version number for tmcd\n";
    print STDERR " -n subnode	Specify the subnode id\n";
    print STDERR " -k keyfile	Specify the private keyfile\n";
    print STDERR " -f datafile	Extra stuff to send to tmcd (tcp mode only)\n";
    print STDERR " -u		Use UDP instead of TCP\n";
    print STDERR " -l path	Use named unix domain socket instead of TCP\n";
    print STDERR " -t timeout	Timeout waiting for the controller.\n";
    print STDERR " -x path	Be a unix domain proxy listening on the named socket\n";
    print STDERR " -X ip:port	Be an inet domain proxy listening on the given IP:port\n";
    print STDERR " -o path	Specify log file name for -x option\n";
    print STDERR " -i   	Do not use SSL protocol\n";
    print STDERR " -c   	Clear tmcc cache first (must be root)\n";
    print STDERR " -D   	Force command to use a direct, UDP request\n";
    print STDERR " -T   	Use TPM\n";
    exit(1);
}
my $optlist	= "ds:p:v:n:k:ul:t:x:X:o:bcDif:T";
my $debug       = 0;
my $CMD;
my $ARGS;

# Prototypes.
sub ParseOptions();

# Drag in path stuff so we can find emulab stuff.
BEGIN { require "/etc/emulab/paths.pm"; import emulabpaths; }

# This library is the guts of the protocol.
use libtmcc;

#
# Turn off line buffering on output
#
$| = 1;

#
# Parse the options. This will set the configuration in the libtmcc library.
#
ParseOptions();

#
# And do it. Dump results to stdout.
#
my @results = ();

if (my $rval = tmcc($CMD, $ARGS, \@results) != 0) {
    exit($rval);
}
if (@results) {
    foreach my $str (@results) {
	print STDOUT "$str";
    }
}
exit(0);

#
# Parse command arguments. Once we return from getopts, all that should be
# left are the required arguments.
#
sub ParseOptions()
{
    my %options = ();
    if (! getopts($optlist, \%options)) {
	usage();
    }
    if (defined($options{"d"})) {
        libtmcc::configtmcc("debug", 1);
	$debug = 1;
    }
    if (defined($options{"b"})) {
        libtmcc::configtmcc("nocache", 1);
    }
    if (defined($options{"i"})) {
        libtmcc::configtmcc("nossl", 1);
    }
    if (defined($options{"T"})) {
        libtmcc::configtmcc("usetpm", 1);
    }
    if (defined($options{"c"})) {
	if ($UID) {
	    print STDERR "Must be root to use the -c option!\n";
	    exit(-1);
	}
        libtmcc::configtmcc("clrcache", 1);
    }
    if (defined($options{"s"})) {
        libtmcc::configtmcc("server", $options{"s"});
    }
    if (defined($options{"p"})) {
        libtmcc::configtmcc("portnum", $options{"p"});
    }
    if (defined($options{"v"})) {
        libtmcc::configtmcc("version", $options{"v"});
    }
    if (defined($options{"s"})) {
        libtmcc::configtmcc("server", $options{"s"});
    }
    if (defined($options{"n"})) {
        libtmcc::configtmcc("subnode", $options{"n"});
    }
    if (defined($options{"k"})) {
        libtmcc::configtmcc("keyfile", $options{"k"});
    }
    if (defined($options{"f"})) {
        libtmcc::configtmcc("datafile", $options{"f"});
    }
    if (defined($options{"u"})) {
        libtmcc::configtmcc("useudp", $options{"u"});
    }
    if (defined($options{"l"})) {
        libtmcc::configtmcc("dounix", $options{"l"});
    }
    if (defined($options{"t"})) {
        libtmcc::configtmcc("timeout", $options{"t"});
    }
    if (defined($options{"x"})) {
        libtmcc::configtmcc("beproxy" , $options{"x"});
    }
    if (defined($options{"X"})) {
        libtmcc::configtmcc("beinetproxy", $options{"X"});
    }
    if (defined($options{"o"})) {
        libtmcc::configtmcc("logfile" , $options{"o"});
    }
    if (defined($options{"D"})) {
        libtmcc::configtmcc("noproxy", 1);
    }
    
    $CMD  = "";
    $ARGS = "";
    if (@ARGV) {
	$CMD = shift(@ARGV);
    }
    if (@ARGV) {
	$ARGS = "@ARGV";
    }
    if ($debug) {
	print STDERR "$CMD $ARGS\n";
    }
}

