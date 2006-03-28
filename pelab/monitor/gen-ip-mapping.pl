#!/usr/bin/perl -w

#
# Make an ipmap file from /etc/hosts
#

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

foreach my $elabnode (keys %elabips) {
    my $plabnode = $elabnode;
    $plabnode =~ s/elab/planet/i;
    if (exists $plabips{$plabnode}) {
        print "$elabips{$elabnode} $plabips{$plabnode} elabc-$elabnode\n";
    }
}

close HOSTS;
