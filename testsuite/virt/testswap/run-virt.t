#!/usr/bin/perl
use SemiModern::Perl;
use TestBed::TestSuite;
use Test::More tests => 1;
use Data::Dumper;

my $pcName = "@NAME@";
my $pcType = "@TYPE@";
my $select = "@SELECT@";
my $runPath = "@RUN_PATH@";
my $resultPath = "@RESULT_PATH@";
my $proj = "@PROJ@";
my $exp = "@EXP@";
my $pairString = "@PAIRS@";

if ($select eq "type")
{
  my @names = get_free_node_names(1, 'type' => $pcType);
  if (scalar(@names) < 1)
  {
      print STDERR "Failed to find a free node of type $pcType. Found "
	  . scalar(@names) . "\n";
      exit(1);
  }
  $pcName = $names[0];
  print STDERR "Selected pc: $pcName\n";
}
else
{
  print STDERR "Using pc: $pcName\n";
}

sub getns
{
    my $numPairs = shift(@_);
    my $pc = shift(@_);
    my $unlimited = shift(@_);

    my $unlimitedText = "";
    if ($unlimited eq "unlimited")
    {
	$unlimitedText = 'tb-set-noshaping $link($i) 1';
    }

    my $ns =
	'source tb_compat.tcl
	set ns [new Simulator]

	set num_pairs '.$numPairs.'

	for {set i 1} {$i <= $num_pairs} {incr i} {
	    set client($i) [$ns node]
	    set server($i) [$ns node]
	    tb-set-node-os $client($i) OPENVZ-64-STD
	    tb-set-node-os $server($i) OPENVZ-64-STD
	    #tb-set-node-failure-action $client($i) "nonfatal"
	    #tb-set-node-failure-action $server($i) "nonfatal"
#	    tb-set-node-tarfiles $client($i) / /proj/tbres/duerig/virt/test-virt.tar.gz
#	    tb-set-node-tarfiles $server($i) / /proj/tbres/duerig/virt/test-virt.tar.gz
	    set "client-$i-agent" [$client($i) program-agent -command "ls"]
	    set "server-$i-agent" [$server($i) program-agent -command "ls"]
	    set link($i) [$ns duplex-link $client($i) $server($i) 10000Kb 0ms DropTail]
            '.$unlimitedText.'
	}

	tb-set-colocate-factor '.($numPairs*2).'

	for {set i 1} {$i <= $num_pairs} {incr i} {
	    tb-fix-node $client($i) '.$pc.'
	    tb-fix-node $server($i) '.$pc.'
	    tb-set-hardware $client($i) pcvm3000
	    tb-set-hardware $server($i) pcvm3000
	}

	$ns rtproto Static
	$ns run';
    return $ns;
}

sub runtest
{
    my $numPairs = shift(@_);
    my $pc = shift(@_);
    my $unlimited = shift(@_);
    my $command = "ssh -t -t ops.emulab.net 'cd $runPath; "
	. "perl parallel-run.pl $runPath $resultPath $proj $exp $numPairs $unlimited'";
    print STDERR $command."\n";
    system($command);
}

my $e = e($exp);
my $ns = getns(1, $pcName, "limited");
print STDERR "Ensuring that experiment is swapped in\n";
eval
{
  $e->modify_ns_swapin_wait($ns);
};

#my @pairList = (1, 2, 5, 10, 15, 20, 30, 40, 50);
my @pairList = split(/ /, $pairString);

my $i = 0;
for ($i = 0; $i < scalar(@pairList); ++$i)
{
    eval
    {
	print STDERR "Swapmod for LIMITED bandwidth scenarios for "
	    . ($pairList[$i]) . " pairs\n";
	$ns = getns($pairList[$i], $pcName, "limited");
	$e->modify_ns_wait($ns);
	print STDERR "Swapmod complete\n";
	runtest($pairList[$i], $pcName, "limited");

	print STDERR "Swapmod UNLIMITED bandwidth scenarios for pair "
	    . ($pairList[$i]) . "\n";
	$ns = getns($pairList[$i], $pcName, "unlimited");
	$e->modify_ns_wait($ns);
	print STDERR "Swapmod complete\n";
	runtest($pairList[$i], $pcName, "unlimited");
    };
}

ok(1);
