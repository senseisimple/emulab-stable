#!/usr/bin/perl

# Runs pings between pairs to
# see test the background base RTT

$usage = <<"END_USAGE";

usage: scan-ping.pl <project> <experiment> <node-count>

END_USAGE

if (@ARGV != 3)
{
    die("Invalid number of arguments: ".@ARGV.$usage);
}

$proj = $ARGV[0];
$exp = $ARGV[1];
$nodeCount = $ARGV[2];

#open NODELIST, "<".$nodeFile;
#@nodes = <NODELIST>;
#$nodeCount = scalar(@nodes);
$serverPort = 1690;
# seconds per run
$duration = 20;

# First argument is the destination node (or 
# Second argument is the command to change to (if "", no change).
sub startProgram
{
    my $source = $_[0];
    my $command = $_[1];
    my $wait = $_[2];
    my $string = "/usr/testbed/bin/tevc ";
    if ($wait != 0)
    {
	$string = $string . '-w ';
    }
    my $string = $string."-e $proj/$exp now node-$source-agent start";
    if ($command ne "")
    {
	$string = $string." COMMAND='".$command."'";
    }
    print("Starting program event: $string\n");
    system($string);
}

sub stopProgram
{
    my $source = $_[0];
    system("/usr/testbed/bin/tevc -e $proj/$exp now node-$source-agent stop");
}

sub runTest
{
    my $source = $_[0];
    my $dest = $_[1];
    my $num = $_[2];
#    startProgram($dest, "sh /bw-bottleneck/multiplex-server.sh $serverPort /bw-bottleneck", 0);
#    sleep(2);
    startProgram($source, "sudo ping -i 0.5 -c 30 end-$dest > /local/logs/$source-$dest.ping", 0);
#sh /bw-bottleneck/multiplex-client.sh $serverPort /bw-bottleneck node-$dest.$exp.$proj.emulab.net $duration $source-to-$dest-run-$run-$num", 0);
    sleep($duration);

#    stopProgram($dest);
    stopProgram($source);
}

#-----------------------------------------------------------------------

$count = 0;

srand($run);

#while (1)
#{
#    $i = int(rand($nodeCount));
#    $j = int(rand($nodeCount));
    for ($i = 1; $i <= $nodeCount; ++$i)
    {
	for ($j = 1; $j <= $nodeCount; ++$j)
	{
	    if ($i != $j)
	    {
		runTest($i, $j, $count);
		++$count;
#	runTest($nodes[$i], $nodes[$j]);
	    }
	}
    }
#    sleep(600);
#}
