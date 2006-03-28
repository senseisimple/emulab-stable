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

my $lines_output = 0;
foreach my $elabnode (keys %elabips) {
    my $plabnode = $elabnode;
    $plabnode =~ s/elab/planet/i;
    if (exists $plabips{$plabnode}) {
        print "$elabips{$elabnode} $plabips{$plabnode} elabc-$elabnode\n";
        $lines_output++;
    }
}

close HOSTS;

if ($lines_output == 0) {
    warn "Hosts file had no names I recognize\n";
    exit 1;
}

exit 0;
