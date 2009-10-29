#!/usr/bin/perl

if (scalar(@ARGV) < 5)
{
    print STDERR "Usage: bw-run.pl <run-path> <result-path> <proj> <exp> <pair-count> <bandwidth [...]>\n";
    print STDERR "The special string 'unlimited' removes bandwidth constraints\n";
    exit(1);
}

$runPrefix = shift(@ARGV);
$resultPrefix = shift(@ARGV);
$proj = shift(@ARGV);
$exp = shift(@ARGV);
$numPairs = shift(@ARGV);
@bwList = @ARGV;

system("mkdir -p $resultPrefix/output");

sub runtest
{
    my $bw = shift(@_);
    my $delay = shift(@_);
    if ($bw ne "unlimited")
    {
	my $command = "perl $runPrefix/network.pl $proj $exp " . $numPairs . " "
	    . $bw . " " . $delay;
	print $command."\n";
	system($command);
    }
    $command = "perl $runPrefix/run.pl $proj $exp $resultPrefix/output/$numPairs-$bw-$delay $runPrefix $numPairs";
    print $command."\n";
    system($command);
}

my @delayList = (0);

my $j = 0;
my $k = 0;
for ($j = 0; $j < scalar(@bwList); ++$j)
{
    for ($k = 0; $k < scalar(@delayList); ++$k)
    {
	print "\n\n\n======BANDWIDTH: " . $bwList[$j] . ", DELAY: "
	    . $delayList[$k]."\n";
	runtest($bwList[$j], $delayList[$k]);
    }
}
