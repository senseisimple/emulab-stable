#!/usr/bin/perl -w

#
# Make an ipmap file from /etc/hosts
#

use Getopt::Std;

my %opt;
getopts('p',\%opt);

use strict;
my (%elabips, %plabips);

open(HOSTS,"/etc/hosts") or die "Unable to open /etc/hosts: $!\n";

while (<HOSTS>) {
    my ($IP,@names) = split /\s+/;
    my $firstname = shift @names;
    if ($firstname =~ /^(.+)-(elink|elan|ecloud|elabc)/) {
        $elabips{$1} = $IP;
    } elsif ($firstname =~ /^(.+)-(plink|plan|pcloud|plabc)/) {
        $plabips{$1} = $IP;
    }
}

my $lines_output = 0;
foreach my $elabnode (keys %elabips) {
    my $plabnode = $elabnode;
    if ($opt{p}) {
        $plabnode =~ s/elab/planet/i;
    } else {
        $plabnode =~ s/elab/plab/i;
    }
    if (exists $plabips{$plabnode} && !$opt{p}) {
        print "$elabips{$elabnode} $plabips{$plabnode} elabc-$elabnode\n";
        $lines_output++;
    } else {
        # Let's hope this is an experiment with a real (not emulated) planetlab
        # half. Note - this should be run by something that has sourced
        # common-env.sh so that these are set
        my $plabhostname = "$elabnode.$ENV{EXPERIMENT}.$ENV{PROJECT}.emulab.net";
        $plabhostname =~ s/^elab/planet/;
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
