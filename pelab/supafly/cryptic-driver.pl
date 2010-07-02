#!/usr/bin/perl

#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#

sub numerically($$);
sub smin(\@);
sub smax(\@);
sub smean(\@);
sub sstddev(\@;$);
sub svariance(\@;$);

use English;

my $CRYPTIC = shift(@ARGV);
my $block_sizes_list = shift(@ARGV);
my $block_count = shift(@ARGV);
my $h = `hostname`;
chomp($h);

my @bss = split(/,/,$block_sizes_list);

foreach my $bs (@bss) {
    my $cmd = "$CRYPTIC -s $bs -l $block_count";

    #print "DEBUG: cmd '$cmd'\n";

    my @output = `$cmd`;
    my @times = ();

    open RDF,">cryptic.raw-$bs.$h" 
	or die "could not open raw dump file\n";

    foreach my $o (@output) {
	chomp($o);
	if ($o =~ /(\d+)\s+(\d+)/) {
	    push @times,"$2";
	    #print "DEBUG: ok '$2'\n";
	}
	else {
	    #print "DEBUG: line '$o'\n";
	}

	print RDF "$o\n";
    }

    close RDF;

    ## stats...
    my $mean = smean(@times);
    my $max = smax(@times);
    my $min = smin(@times);
    my $sdev = sstddev(@times,$mean);
    my $svar = svariance(@times,$sdev);

    printf "bs=%d min=%.2f max=%.2f mean=%.2f stddev=%.2f var=%.2f\n",$bs,$min,$max,$mean,$sdev,$svar;

}




## subs...
sub numerically($$) { $x = shift; $y = shift; $x <=> $y }

sub smin(\@) {
    my $dref = shift;
    my $min = 4000000000;
    foreach my $i (@{$dref}) {
        my $n = 0.0 + $i;
        if ($i < $min) {
            $min = $i;
        }
    }

    return $min;
}

sub smax(\@) {
    my $dref = shift;
    my $max = -4000000000;
    foreach my $i (@{$dref}) {
        my $n = 0.0 + $i;
        if ($i > $max) {
            $max = $i;
        }
    }

    return $max;
}

sub smean(\@) {
    my $dref = shift;
    my $mean = 0;
    foreach my $i (@{$dref}) {
        $mean += (0.0 + $i);
    }
    $mean = $mean / scalar(@{$dref});

    return $mean;
}

sub sstddev(\@;$) {
    my $dref = shift;
    my $mean = shift;
    if (!$mean) {
        $mean = smean(@{$dref});
    }
    my $ssum = 0;
    foreach my $i (@{$dref}) {
        $ssum += ($i-$mean) ** 2;
    }

    my $stddev = sqrt($ssum/scalar(@{$dref}));

    return $stddev;
}

sub svariance(\@;$) {
    my $dref = shift;
    my $stddev = shift;
    if (!$stddev) {
        $stddev = sstddev(@{$dref});
    }

    my $variance = $stddev ** 2;

    return $variance;
}
