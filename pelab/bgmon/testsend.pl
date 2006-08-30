#!/usr/bin/perl


use libwanetmon;
use IO::Socket::INET;
use IO::Select;
use Getopt::Std;
use strict;

sub usage {
    warn "Usage: $0 [-p port] [-e pid/eid] [-d testduration] <monitoraddr> <link dest> ".
	"<testtype> <testper(seconds)> <COMMAND>\n";
    return 1;
}

my %opt = ();
getopts("e:d:p:h",\%opt);

my ($expt, $port, $testduration);
if ($opt{e}) { $expt = $opt{e}; } else { $expt = "__none"; }
if ($opt{d}) { $testduration = $opt{d}; }
if ($opt{p}) { $port = $opt{p}; } else{ $port = 5052; }
if ($opt{h}) { exit &usage; }
if (@ARGV != 5) { exit &usage; }

setcmdport($port);

my ($srcnode, $destnode, $testtype, $testper, $cmd) = @ARGV;


my %cmd = ( expid     => $expt,
	    cmdtype   => $cmd,
	    dstnode  => $destnode,
	    testtype  => $testtype,
	    testper   => $testper,
	    duration  => $testduration
	    );

my $socket;
my $sel = IO::Select->new();


sendcmd( $srcnode, \%cmd );


exit(0);

