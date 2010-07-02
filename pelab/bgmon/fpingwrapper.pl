#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#


use strict;

my %params = (@ARGV);
my $execpath = "/tmp/fping";
my %ERRID = (  #kept the same as defined in libwanetmon for backwards compatibility
    unknown => -3,
    timeout => -1,
    unknownhost => -4,
    ICMPunreachable => -5
    );

#
# Check for existence of all required params
#
# TODO

#
# Add optional args, if they exist
#
my %optional_exec_args; #TODO

#
# Form arguments specific to this execuable
#
my $execargs = " -t $params{timeout} -s -r $params{retries} $params{target}";


#
# Execute testing application
#
#print "FPING WRAPPER... executing= $execpath $execargs\n";
my $raw = `sudo $execpath $execargs 2>&1`;
#print "FPING WRAPPER... parsing raw= $raw\n";

#
# Parse output
#
$_ = $raw;
my $measr = 0;
my $error = 0;

if( /^ICMP / )
{
    $error = $ERRID{ICMPunreachable};
}elsif( /address not found/ ){
    $error = $ERRID{unknownhost};
}elsif( /2 timeouts/ ){
    $error = $ERRID{timeout};
}elsif( /[\s]+([\S]*) ms \(avg round trip time\)/ ){
    $measr = "$1" if( $1 ne "0.00" );
}else{
    $error = $ERRID{unknown};
}


#print "MEASR=$measr, ERROR=$error\n";


#
# Output result
#
print "latency=$measr,error=$error\n";
