#!/usr/bin/perl

sub printLog
{
    my $line = shift(@_);
    my $time = `date +%F_%H:%M:%S`;
    chomp($time);
    print STDERR  $time . " " . $line . "\n";
}

if (scalar(@ARGV) != 4)
{
    print STDERR "Usage: client.pl <barrier-name> <pair-number> <duration (s)> <run-path>\n";
    exit(1);
}

$DELAY = 1;

$name = $ARGV[0];
$num = $ARGV[1];
$duration = $ARGV[2];
$runPrefix = $ARGV[3];

printLog("$num Client Begin");

printLog("$num Client Before server barrier");
$status = (system("$runPrefix/run-sync.pl 30 -n $name-server") >> 8);

if ($status == 240)
{
    exit(1);
}

printLog("$num Client Before iperf");
system("/usr/local/etc/emulab/emulab-iperf -c server-$num -w 256k -i $DELAY -p 4000 -t $duration");

printLog("$num Client Before client barrier");
system("$runPrefix/run-sync.pl $duration -n $name-client");

printLog("$num Client End");
