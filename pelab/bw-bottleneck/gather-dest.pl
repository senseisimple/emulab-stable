#!/usr/bin/perl

# Runs simultaneous iperfs from each pair of hosts in the host file to
# the dest. Creates a directory structure and saves the tcpdump files
# to that structure.

$usage = <<"END_USAGE";

usage: gather-dest.pl dest <host-file>
  Where <host-file> is the path of a file of hosts (one hostname per line)
  to be checked and dest is one of those hosts.
END_USAGE

$serverPort = 1690;

# non-blocking system call.
# returns the child process pid.
sub nbSystem
{
    my $command = $_[0];
    my $pid = fork();
    if ($pid == 0)
    {
	exit(system($command));
    }
    elsif ($pid > 0)
    {
	return $pid;
    }
    else
    {
	return 0;
    }
}

sub remoteSystem
{
    my $blocking = $_[0];
    my $dest = $_[1];
    my $command = $_[2];
    my $result = 0;
    my $string = "ssh -o BatchMode=yes -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -lutah_pelab $dest $command";
    if ($blocking != 0)
    {
	system($string);
    }
    else
    {
	$result = nbSystem($string);
	sleep(1);
    }
    return $result;
}

sub runTest
{
    my $source1 = $_[0];
    my $source2 = $_[1];
    my $dest = $_[2];
    my @pids;
    my $i = 0;
    # TODO: You are here. Upload an iperf before anything else.
    # TODO: Change tcpdump to dump to file rather than to output to stdout.
    # Run tcpdump on every machine.
    for ($i = 0; $i < 3; ++$i)
    {
	push(@pids, remoteSystem(0, $_[$i], "sudo tcpdump -i vnet dst port $serverPort"));
    }
    sleep(5);
    push(@pids, remoteSystem(0, $source1, "iperf -c $dest -p $serverPort"));
    push(@pids, remoteSystem(1, $source2, "iperf -c $dest -p $serverPort"));
    for ($i = 0; $i < @pids; ++$i)
    {
	if ($pids[$i] != 0)
	{
	    kill($pids[$i], SIGTERM);
	}
    }
    # Copy tcpdump logs back to us.
    # Remove logs from node
}

if (@ARGV != 2)
{
    die("Invalid number of arguments: ".@ARGV.$usage);
}

$dest = $ARGV[0];
open(HOSTS, "<".$ARGV[1]) or die("Could not open host file".$usage);
@sourceList = <HOSTS>;

$iperfServerPid = remoteSystem(0, $dest, "iperf -s -p $serverPort");
if ($iperfServerPid != 0)
{
    for ($i = 0; $i < @sourceList; ++$i)
    {
	for ($j = $i + 1; $j < @sourceList; ++$j)
	{
	    if ($sourceList[$i] ne $dest
		&& $sourceList[$j] ne $dest)
	    {
		runTest($sourceList[$i], $sourceList[$j], $dest);
	    }
	}
    }
    kill($iperfServerPid, SIGTERM);
}
