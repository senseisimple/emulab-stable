#!/usr/bin/perl

# Runs iperfs between random pairs at each of 10 multiplex levels to
# see how the background traffic

$usage = <<"END_USAGE";

usage: scan-multiplex.pl <project> <experiment> <node-count> <run-number>

END_USAGE

if (@ARGV != 4)
{
    die("Invalid number of arguments: ".@ARGV.$usage);
}

$proj = $ARGV[0];
$exp = $ARGV[1];
$nodeCount = $ARGV[2];
$run = $ARGV[3];

#open NODELIST, "<".$nodeFile;
#@nodes = <NODELIST>;
#$nodeCount = scalar(@nodes);
$serverPort = 1690;
# seconds per run
$duration = 60;

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
    startProgram($dest, "sh /bw-bottleneck/multiplex-server.sh $serverPort /bw-bottleneck", 0);
    sleep(2);
    startProgram($source, "sh /bw-bottleneck/multiplex-client.sh $serverPort /bw-bottleneck node-$dest.$exp.$proj.emulab.net $duration $source-to-$dest-run-$run-$num", 0);
    sleep($duration * 11);

    stopProgram($dest);
    stopProgram($source);
}

#-----------------------------------------------------------------------

$count = 0;

srand($run);

while (1)
{
    $i = int(rand($nodeCount));
    $j = int(rand($nodeCount));
    if ($i != $j)
    {
	runTest($i, $j, $count);
	++$count;
#	runTest($nodes[$i], $nodes[$j]);
    }
    sleep(600);
}
