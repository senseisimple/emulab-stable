#!/usr/bin/perl

# Runs simultaneous iperfs from each pair of hosts in the host file to
# the dest. Creates a directory structure and saves the tcpdump files
# to that structure.

$usage = <<"END_USAGE";

usage: gather-dest.pl <project> <experiment> <node-count> <source-number> <duration>

END_USAGE

if (@ARGV != 5)
{
    die("Invalid number of arguments: ".@ARGV.$usage);
}

$project = $ARGV[0];
$exp = $ARGV[1];
$nodeCount = $ARGV[2];
$source = $ARGV[3];
$duration = $ARGV[4];
$serverPort = 1690 + $source;

# First argument is the destination node (or 
# Second argument is the command to change to (if "", no change).
sub startProgram
{
    my $dest = $_[0];
    my $command = $_[1];
    my $wait = $_[2];
    my $string = "/usr/testbed/bin/tevc ";
    if ($wait != 0)
    {
	$string = $string . '-w ';
    }
    my $string = $string."-e $project/$exp now node-$source-to-node-$dest start";
    if ($command ne "")
    {
	$string = $string." COMMAND='".$command."'";
    }
    print("Starting program event: $string\n");
    system($string);
}

sub stopProgram
{
    my $dest = $_[0];
    system("/usr/testbed/bin/tevc -e $project/$exp now node-$source-to-node-$dest stop");
}

sub runTest
{
    my $dest1 = $_[0];
    my $dest2 = $_[1];
    my $i = 0;

    stopProgram($dest1);
    stopProgram($dest2);
    stopProgram("client");

    startProgram($dest1, "sh /bw-bottleneck/run-server.sh $serverPort /bw-bottleneck dump-$source-$dest1-$dest2.dump", 0);
    startProgram($dest2, "sh /bw-bottleneck/run-server.sh $serverPort /bw-bottleneck dump-$source-$dest1-$dest2.dump", 0);

    sleep(2);

    startProgram("client", "sh /bw-bottleneck/run-client.sh node-$dest1.$exp.$project.emulab.net node-$dest2.$exp.$project.emulab.net $serverPort /bw-bottleneck dump-$source-$dest1-$dest2.dump $duration", 1);

    stopProgram($dest1);
    stopProgram($dest2);
    stopProgram("client");
}


#-----------------------------------------------------------------------

for ($i = 1; $i <= $nodeCount; ++$i)
{
    for ($j = $i + 1; $j <= $nodeCount; ++$j)
    {
	if ($i != $source
	    && $j != $source)
	{
	    runTest($i, $j);
	}
    }
}
