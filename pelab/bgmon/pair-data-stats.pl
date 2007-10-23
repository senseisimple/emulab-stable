#!/usr/bin/perl

#
# Get stats of available pair data for a set of nodes over a fixed
# interval.  Useful to see if managerclient.pl is working.
#


use strict;
use warnings 'all';
no warnings 'uninitialized';

use Statistics::Descriptive;
use Date::Parse;

use Data::Dumper;

use DBI;

if (@ARGV < 3) {
  print "Usage: $0 <date> <interval in secs> <plab nodes> ...\n";
  exit 1;
}

my $start_d = shift @ARGV;
my $dur = shift @ARGV;

my $start = str2time($start_d) - 5;
my $stop  = $dur > 0 ? $start + $dur + 10 : -1;

my @nodes = @ARGV;
my $nodes = join(',', map {"'$_'"} @nodes);

my $dbh = DBI->connect("DBI:mysql:database=pelab", "flexlabdata");

my $h = $dbh->prepare("SELECT s.node_id as src_node, d.node_id as dst_node, pd.* ".
		      "  from site_mapping as s, site_mapping as d, pair_data as pd".
		      "  where s.site_idx = pd.srcsite_idx".
                      "    and s.node_idx = pd.srcnode_idx".
		      "    and d.site_idx = pd.dstsite_idx".
                      "    and d.node_idx = pd.dstnode_idx".
		      "    and (   s.node_id in ($nodes) ".
		      "        and d.node_id in ($nodes))".
		      "    and unixstamp > $start ".
		      ($stop > 0 ? "    and unixstamp < $stop " : "").
		      "  order by unixstamp");

$h->execute;

my %tally;

my $c = 0;
my $what;
my $value;
while (my $d = $h->fetchrow_hashref) {
  $c++;
  my $date = localtime($d->{unixstamp});
  my $pair = "$d->{src_node} $d->{dst_node}";
  if (defined $d->{latency}) {
    $what = "L";
    $value = $d->{latency};
  } elsif (defined $d->{bw}) {
    $what = "B";
    $value = $d->{bw};
  }
  push @{$tally{$what}{$pair}}, [$d->{unixstamp}, $value];
  print "$pair : $date ($d->{unixstamp}) : $what : $value\n";
}
print "COUNT: $c\n";

sub print_stats ($$$) ;

foreach my $what ('L', 'B') {
  print "\n*** $what\n";
  my $total = 0;
  my $missing_t = 0;
  my $missing_both_t = 0;
  my @stats;
  foreach my $i (0 .. 3) {
    $stats[$i] = Statistics::Descriptive::Full->new();
  }
  my %all;
  foreach my $s (sort @nodes) {
    foreach my $d (sort @nodes) {
      ++$total;
      next unless $s lt $d;
      printf "%7s %7s :: ", $s, $d;
      my $i = 0;
      my $missing = 0;
      my @data;
      foreach my $pair ("$s $d", "$d $s") {
	if (! exists $tally{$what}{$pair}) {
	  print " . . . . . . . . : ";
	  $data[$i] = [0, 0, 0, 0];
	  ++$missing;
	  ++$i;
	  next;
	}
	my @d = @{$tally{$what}{$pair}};
 	my $valid = 0;
 	my $errors = 0;
	foreach my $d (@d) {
	  if ($d->[1] < 0) {++$errors;}
	  else             {++$valid;}
	}
	@d = map {$_->[0]} @d;
	my $prev = shift @d;
	my @diffs;
	foreach my $d (@d) {
	  my $diff = $d - $prev;
	  push @diffs, $diff;
	  $prev = $d;
	}
	my $total_diff = 0;
	foreach my $d (@diffs) {
	  $total_diff += $d;
	}
	my $avg_diff = @diffs ? $total_diff / (@diffs + 0) : 0;
	$data[$i] = [$valid + $errors, $errors, $total_diff, $avg_diff];
	printf "%2d [%2d] %4d %3d : ", $valid + $errors , $errors, $total_diff, $avg_diff;
	++$i;
      }
      if ($what eq 'L') {
	my $use = $data[0][0] > $data[1][0] ? 0 : 1;
	foreach my $j (0 .. 3) {
	  $stats[$j]->add_data( $data[$use][$j] );
	}
      } else {
	foreach my $i (0 .. 1) {
	  foreach my $j (0 .. 3) {
	    $stats[$j]->add_data( $data[$i][$j] );
	  }
	}
      }
      $missing_t += $missing;
      if ($missing == 2) {$missing_both_t++;}
      print "\n";
    }
  }
  print "\n";
  if ($what eq 'L') {
    print "NOTE: Stats only for the direction with the highest count\n";
  }
  print_stats(\@stats, "mean", sub {$_[0]->mean()});
  print_stats(\@stats, "std dev", sub {$_[0]->standard_deviation()});
  print_stats(\@stats, "median", sub {$_[0]->median()});
  print_stats(\@stats, "min", sub {$_[0]->min()});
  print_stats(\@stats, " 5%", sub {$_[0]->percentile(5)});
  print_stats(\@stats, "25%", sub {$_[0]->percentile(25)});
  print_stats(\@stats, "50%", sub {$_[0]->median()});
  print_stats(\@stats, "75%", sub {$_[0]->percentile(75)});
  print_stats(\@stats, "95%", sub {$_[0]->percentile(95)});
  print_stats(\@stats, "max", sub {$_[0]->max()});
  if ($what eq 'L') {
    printf "MISSING BOTH WAYS: %d / %d = %0.3f\n", $missing_both_t, $total/2, $missing_both_t/($total/2);
  }
  printf "MISSING: %d / %d = %0.3f\n", $missing_t, $total, $missing_t/$total;
}

sub print_stats ( $$$ ) 
{
  my ($s, $desc, $f) = @_;
  printf("%8s :: %4.1f [%4.1f] %6.1f %6.1f\n", $desc, 
	 $f->($s->[0]) + 0,
	 $f->($s->[1]) + 0,
	 $f->($s->[2]) + 0,
	 $f->($s->[3]) + 0);
}






