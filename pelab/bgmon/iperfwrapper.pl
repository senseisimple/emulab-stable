#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#


use strict;

my %params = (@ARGV);
my $execpath = "/tmp/iperf";
my %ERRID = ( #kept the same as defined in libwanetmon for backwards compatibility
    unknown => -3,
    timeout => -1,
    unknownhost => -4,
    iperfHostUnreachable => -6 
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
my $execargs = " -fk -c $params{target} -t $params{duration} -p $params{port}";

#
# Execute testing application
#
my $raw = `$execpath $execargs`;
#print "RAW = $raw\n";

#
# Parse output
#
$_ = $raw;
my $measr = 0;
my $error = 0;

if(    /connect failed: Connection timed out/ ){
    # this one shouldn't happen, if the timeout check done by
    # bgmon is set low enough.
    $error = $ERRID{timeout};
}elsif( /write1 failed:/ ){
    $error = $ERRID{iperfHostUnreachable};
}elsif( /error: Name or service not known/ ){
    $error = $ERRID{unknownhost};
}elsif( /\s+(\S*)\s+([MK])bits\/sec/ ){
    $measr = $1;
    if( $2 eq "M" ){
        $measr *= 1000;
    }
}else{
    $error = $ERRID{unknown};
}

#print "MEASR=$measr, ERROR=$error\n";


#
# Output result
#
print "bw=$measr,error=$error\n";
