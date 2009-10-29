#!/usr/bin/perl

if (scalar(@ARGV) != 5)
{
    print STDERR "Usage: parallel-run.pl <run-path> <result-path> <proj>\n"
	."    <exp> <pairCount> <unlimited | limited>\n";
    exit(1);
}

my $runPath = shift(@ARGV);
my $resultPrefix = shift(@ARGV);
my $proj = shift(@ARGV);
my $exp = shift(@ARGV);
my $pairCount = shift(@ARGV);
my $unlimited = shift(@ARGV);

system("mkdir -p $resultPrefix/log");

my @bwList = (500, 1000, 2000, 5000, 10000, 20000, 50000, 100000, 200000);
#my @bwList = (500, 1000);
my $bwString = join(" ", @bwList);
if ($unlimited eq "unlimited")
{
    $bwString = "unlimited";
}

#my $i = 0;
#for ($i = 0; $i < scalar(@pairList); ++$i)
#{
#    my $pairCount = $pairList[$i];
    my $command = "perl bw-run.pl $runPath $resultPrefix $proj $exp "
	. "$pairCount $bwString | tee $resultPrefix/log/$pairCount.log";
    system($command);
#}
