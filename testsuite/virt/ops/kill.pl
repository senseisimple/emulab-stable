#
# EMULAB-COPYRIGHT
# Copyright (c) 2009 University of Utah and the Flux Group.
# All rights reserved.
#
if (scalar(@ARGV) != 3)
{
    print STDERR "Usage: kill.pl <proj> <exp> <pairCount>\n";
    exit(1);
}

$proj = $ARGV[0];
$exp = $ARGV[1];
$pairCount = $ARGV[2];

sub killProgram
{
    my $agentName = shift(@_);
    my $string = "/usr/testbed/bin/tevc ";
    my $string = $string."-e $proj/$exp now $agentName stop";
    system($string);
}

sub killAll
{
    my $i = 1;
    killProgram("vhost-0_program");
    for ($i = 1; $i <= $pairCount; ++$i)
    {
	killProgram("client-$i-agent");
	killProgram("server-$i-agent");
    }
}

killAll();
