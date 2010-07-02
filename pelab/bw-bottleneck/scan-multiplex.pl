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
$duration = 30;

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
    startProgram($dest, "sh /local/bw-bottleneck-wavelet/multiplex-server.sh $serverPort /local/bw-bottleneck-wavelet", 0);
    sleep(2);
    startProgram($source, "sh /local/bw-bottleneck-wavelet/multiplex-client.sh $serverPort /local/bw-bottleneck-wavelet node-$dest.$exp.$proj.emulab.net $duration $source-to-$dest-run-$run-$num", 0);
    sleep($duration * 11);

    stopProgram($dest);
    stopProgram($source);
}

#-----------------------------------------------------------------------

$count = 0;

srand($run);

my %PairHash = ();
my $iterLimit = $nodeCount*($nodeCount-1);

while (1)
{
    $i = int(rand($nodeCount)) + 1;
    $j = int(rand($nodeCount)) + 1;

    if( keys(%PairHash) >= $iterLimit )
    {
        last;
    }

    if ($i != $j)
    {
        my $nodeString = $i . ":" . $j;
        if( not ( $PairHash{$nodeString} ) )
        {
            $PairHash{$nodeString} = 1;
            runTest($i, $j, $count);
            ++$count;

            sleep(30);
        }
        else
        {
            next;
        }
    }
}
#    sleep(600);
#}
