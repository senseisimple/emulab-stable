#!/usr/bin/perl
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#

use strict;

open FILE, "< $ARGV[0]"
    or die "Can't open file";

my @expnodes = <FILE>;
foreach my $line (@expnodes){
#    $_ = $line;
    if( $line =~ /\((plab\d+)\)/ ){
	print $1."\n";
    }
}
