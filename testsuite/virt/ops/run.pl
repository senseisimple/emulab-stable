#!/usr/bin/perl

use POSIX ":sys_wait_h";

if (scalar(@ARGV) != 5)
{
    print STDERR "Usage: run.pl <proj> <exp> <result-path> <run-path> <pair-count>\n";
    exit(1);
}

$serverNode = "client-1";
$duration = 300;

$proj = $ARGV[0];
$exp = $ARGV[1];
$prefix = $ARGV[2];
$runPrefix = $ARGV[3];
$pairCount = $ARGV[4];

$syncName = "vbarrier";

sub startProgram
{
    my $agentName = shift(@_);
    my $command = shift(@_);
    my $wait = shift(@_);
    my $string = "/usr/testbed/bin/tevc ";
    if ($wait != 0)
    {
        $string = $string . '-w ';
    }
    my $string = $string."-e $proj/$exp now $agentName start";
    if ($command ne "")
    {
        $string = $string." COMMAND='".$command."'";
    }
#    print("Starting program event: $string\n");
    system($string);
}

sub killProgram
{
    my $agentName = shift(@_);
    my $string = "/usr/testbed/bin/tevc ";
    my $string = $string."-e $proj/$exp now $agentName stop";
    system($string);
}

# Returns 0 for success and nonzero for failure
sub waitForSync
{
    my $count = shift(@_);
    my $name = shift(@_);
    my $timeout = shift(@_);
    my $result = system("ssh -o StrictHostKeyChecking=no "
			. "-o UserKnownHostsFile=/dev/null "
			. "$serverNode.$exp.$proj.emulab.net "
			. "$runPrefix/command.pl $runPrefix/run-sync.pl "
			. "$timeout "
			. "-i $count -n $name");
    return ($result >> 8);
}

sub resetSync
{
    system("ssh -t -t -o StrictHostKeyChecking=no "
	   . "-o UserKnownHostsFile=/dev/null "
	   . "$serverNode.$exp.$proj.emulab.net "
	   . "$runPrefix/command.pl $runPrefix/reset-syncd.pl");
}

sub printLog
{
    my $line = shift(@_);
    print $line;
}

sub runTest
{
    my $i = 1;
    printLog("Wiping the slate clean\n");
    killProgram("vhost-0_program");
    for ($i = 1; $i <= $pairCount; ++$i)
    {
	killProgram("client-$i-agent");
	killProgram("server-$i-agent");
    }
    sleep(1);
    resetSync();

    printLog("Starting monitor\n");
    startProgram("vhost-0_program",
		 "perl $runPrefix/monitor.pl $syncName $duration $runPrefix",
		 0);
    printLog("Starting clients and servers\n");
    for ($i = 1; $i <= $pairCount; ++$i)
    {
	startProgram("client-$i-agent",
		     "perl $runPrefix/client.pl $syncName $i $duration $runPrefix",
		     0);
	startProgram("server-$i-agent",
		     "perl $runPrefix/server.pl $syncName $i $duration $runPrefix",
		     0);
    }
    my $totalCount = $pairCount*2 + 1;
    printLog("Waiting for server barrier\n");
    my $waited = waitForSync($totalCount, "$syncName-server", 30);
    if ($waited != 0)
    {
	printLog("Skipping client barrier\n");
    }
    else
    {
	printLog("Waiting for client barrier\n");
	waitForSync($totalCount, "$syncName-client", $duration*2);
    }
}

sub gatherResults
{
    my $i = 1;
    my $list = "vhost-0 ";
    for ($i = 1; $i <= $pairCount; ++$i)
    {
	$list = $list . " client-$i server-$i";
    }
    printLog("Gathering logs\n");
    system("/proj/tbres/duerig/frisbee/loghole -e $proj/$exp sync $list");
    printLog("Copying logs\n");
    system("mkdir -p $prefix");
    system("cp -R /proj/$proj/exp/$exp/logs $prefix");
    printLog("Cleaning old logs\n");
    system("/proj/tbres/duerig/frisbee/loghole -e $proj/$exp clean -f");
}

sub trial
{
    runTest();
    gatherResults();
}

trial();
