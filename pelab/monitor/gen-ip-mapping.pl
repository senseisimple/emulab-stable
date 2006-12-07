#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#

#
# Make an ipmap file from /etc/hosts
#

use Getopt::Std;

my %opt;
getopts('p',\%opt);

use strict;
my (%elabips, %plabips, %planetnodes);

open(HOSTS,"/etc/hosts") or die "Unable to open /etc/hosts: $!\n";

while (<HOSTS>) {
    my ($IP,@names) = split /\s+/;
    my $firstname = shift @names;
    if ($firstname =~ /^(.+)-(elink|elan|ecloud|elabc)$/) {
        $elabips{$1} = $IP;
    } elsif ($firstname =~ /^(.+)-(plink|plan|pcloud|plabc)$/) {
        $plabips{$1} = $IP;
    } elsif ($firstname =~ /^(.+)-realinternet$/) {
        # Note - we don't really get the planetlab node's IP address in the
        # hosts file, so we just use this to find out if the node exists at all
        $planetnodes{$1} = 1;
    }
}

my $lines_output = 0;
foreach my $elabnode (keys %elabips) {
    my $plabnode = $elabnode;
    my $planetnode = $elabnode;
    $plabnode =~ s/elab/plab/i;
    $planetnode =~ s/elab/planet/i;
    #if ($opt{p}) {
    #    $plabnode =~ s/elab/planet/i;
    #} else {
    #    $plabnode =~ s/elab/plab/i;
    #}
    if (exists $plabips{$plabnode}&& !$opt{p}) {
        print "$elabips{$elabnode} $plabips{$plabnode} elabc-$elabnode\n";
        $lines_output++;
    } elsif (exists $plabips{$planetnode}&& !$opt{p}) {
        print "$elabips{$elabnode} $plabips{$planetnode} elabc-$elabnode\n";
        $lines_output++;
    } else {
        # If there is not a corresponding planet-* node in the hosts file, it
        # has probably been removed from the experiment, and we should skip ip
        my $plabnode = $elabnode;
        $plabnode =~ s/^elab/planet/;
        if (! exists($planetnodes{$plabnode})){
            warn "Skipping $plabnode, which doesn't exist\n";
            next;
        }
        # Let's hope this is an experiment with a real (not emulated) planetlab
        # half. Note - this should be run by something that has sourced
        # common-env.sh so that these enviroment variables are set
        my $plabhostname = "$plabnode.$ENV{EXPERIMENT}.$ENV{PROJECT}.emulab.net";
        # Find out the IP address for $plabhostname
        open(H,"host $plabhostname |") or die "Unable to run host\n";
        # Yuck. Why can't 'host' have a flag to spit out just the IP?
        <H>; # Throw away first line
        # The next line is the one with interesting data
        my $interesting_line = <H>;
        close H;
        if ($interesting_line =~ /has address (.*)$/) {
            my $plabIP = $1;
            print "$elabips{$elabnode} $plabIP elabc-$elabnode\n";
            $lines_output++;
        }
    }
}

close HOSTS;

if ($lines_output == 0) {
    warn "Hosts file had no names I recognize\n";
    exit 1;
}

exit 0;
