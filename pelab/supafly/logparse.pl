#!/usr/bin/perl -w

use English;
use Getopt::Std;
#
# A simple script to parse a bunch of logfiles, warn about a few unexpected
# conditions, and dump data to make graphs.
#

my $block_count = 0;
my $msg_count = 0;
my $frag_count = 0;
my $outputdir = '.';
my $basename = 'supafly.logparse';

sub usage {
    print "USAGE: -mbfOo middlemanlog,sendlog,recvlog ...\n";
    print "  -m  Expected number of messages.\n";
    print "  -b  Expected number of blocks.\n";
    print "  -f  Expected number of fragments.\n";
    print "  -O  Output directory.\n";
    print "  -o  Base output filename.\n";
    
    exit -1;
}

# grab args:
my $optlist = "m:b:f:O:o:";

my %options = ();
if (!getopts($optlist,\%options)) {
    usage();
}

if (defined($options{'m'})) {
    $msg_count = $options{'m'};
}
else {
    print "ERROR: must supply expected number of msgs!\n";
    usage();
}
if (defined($options{'b'})) {
    $block_count = $options{'b'};
}
else {
    print "ERROR: must supply expected number of blocks!\n";
    usage();
}
if (defined($options{'f'})) {
    $frag_count = $options{'f'};
}
else {
    print "ERROR: must supply expected number of fragments!\n";
    usage();
}
if (defined($options{'O'})) {
    $outputdir = $options{'O'};
}
if (defined($options{'o'})) {
    $basename = $options{'o'};
}

my @logs = ();

if (!scalar(@ARGV)) {
    print "ERROR: must supply logfiles!\n";
    exit -1;
}
else {
    while (scalar(@ARGV)) {
	push @logs, shift(@ARGV);
    }
}

#
# Data structure that we're going to build:
# bs = hash;
# bs{'m%d b%d f%d'}{'M_read'} = unixtime;
#                  {'M_endcomp'} = unixtime;
#                  {'M_send'} = unixtime;
#                  {'M_cryptotime'} = timediff_us;
#                  {'M_ack'} = unixtime;
#                  {'M_cryptoop'} = str;
#
#                  {'S_send'} = ;
#                  {'S_ack'} = ;
#
#                  {'R_recv'} = ;
#                  {'R_ack'} = ;
#
# bsList = ()  # list of bs's corresponding to the input logfile tuples
#

$debug = 1;

sub debug {
    if ($debug) {
	print "DEBUG: " . shift;
#	my $s = sprintf @_;
#	print "$s";
    }
}

my @bsList = ();
my $line = '';
my ($mlc,$slc,$rlc) = (0,0,0);

foreach my $logtuple (@logs) {
    my ($mlog,$slog,$rlog) = split(/,/,$logtuple);

    my %bs = ();

    debug("starting on logs $mlog $slog $rlog:\n");

    open(ML,"$mlog") or die "couldn't open $mlog!";
    while ($line = <ML>) {
	#BLOCKTIME(de): m0 b0 f0 1112; read=1172014045.737621; compute=1172014045.739196; send=1172014045.739301
	if ($line =~ /BLOCKTIME\((\w+)\).+m(\d+).+b(\d+).+f(\d+)\s+(\d+);.+read\=(\d+\.\d+).+compute\=(\d+\.\d+).+send\=(\d+\.\d+)/) {
	    $bs{"m$2 b$3 f$4"}{'M_cryptotime'} = $5 + 0;
	    #debug("ctime = " . $bs{"m$2 b$3 f$4"}{'M_cryptotime'} . "\n");
	    $bs{"m$2 b$3 f$4"}{'M_read'} = $6 + 0.0;
	    $bs{"m$2 b$3 f$4"}{'M_endcomp'} = $7 + 0.0;
	    $bs{"m$2 b$3 f$4"}{'M_send'} = $8 + 0.0;
	    $bs{"m$2 b$3 f$4"}{'M_cryptoop'} = $1;
	    #debug("a");
	    ++$mlc;
	}
	elsif ($line =~ /ACKTIME: recv m(\d+) b(\d+) f(\d+) at (\d+\.\d+)/) {
	    $bs{"m$1 b$2 f$3"}{'M_ack'} = $4 + 0.0;
	    ++$mlc;
	}
    }
    close(ML);
    
    open(SL,"$slog") or die "couldn't open $slog!";
    while ($line = <SL>) {
	if ($line =~ /ACKTIME m(\d+) b(\d+) f(\d+) (\d+\.\d+)/) {
	    $bs{"m$1 b$2 f$3"}{'S_ack'} = $4 + 0.0;
	    ++$slc;
	}
	elsif ($line =~ /TIME m(\d+) b(\d+) f(\d+) (\d+\.\d+)/) {
	    $bs{"m$1 b$2 f$3"}{'S_send'} = $4 + 0.0;
	    ++$slc;
	}
    }
    close(SL);

    open(RL,"$rlog") or die "couldn't open $rlog!";
    while ($line = <RL>) {
	if ($line =~ /ACKTIME: sent m(\d+) b(\d+) f(\d+) at (\d+\.\d+)/) {
	    $bs{"m$1 b$2 f$3"}{'R_ack'} = $4 + 0.0;
	    ++$rlc;
	}
	elsif ($line =~ /TIME m(\d+) b(\d+) f(\d+) (\d+\.\d+)/) {
	    $bs{"m$1 b$2 f$3"}{'R_recv'} = $4 + 0.0;
	    ++$rlc;
	}
    }
    close(RL);

    debug("read $mlc,$slc,$rlc valid lines from $logtuple\n");

    my $rtts = 0;
    my $cryptotime_count = 0;

    # try to figure out for each msg/block/frag chunk, the recv->send time,
    # and how much of that time was compute time.
    my %tstats = ();
    for (my $mi = 0; $mi < $msg_count; ++$mi) {
	for (my $bi = 0; $bi < $block_count; ++$bi) {
	    for (my $fi = 0; $fi < $frag_count; ++$fi) {
		# calc time!
		my $str = "m$mi b$bi f$fi";
		#debug("str = $str\n");
		if (defined($bs{$str}{'S_ack'})
		    && defined($bs{$str}{'S_send'})) {
		    # rtt, including cputime, in microseconds
		    $tstats{$str}{'rtt'} = int(($bs{$str}{'S_ack'} - 
			$bs{$str}{'S_send'}) * 1000000.0);
		    # est oneway as rtt/2, then add back in half cputime.
		    $tstats{$str}{'oneway'} = int((($bs{$str}{'S_ack'} - 
		        $bs{$str}{'S_send'}) * 1000000.0) / 2 + 
			$bs{$str}{'M_cryptotime'}/2);

		    ++$rtts;
		    
#		    $tstats{$str}{'oneway'} = int(($bs{$str}{'R_recv'} - 
#			$bs{$str}{'S_send'}) * 1000000.0);
#		    $tstats{$str}{'oneway'} = $bs{$str}{'R_recv'} - 
#			$bs{$str}{'S_send'};
		    debug("$str oneway time = " . $tstats{$str}{'oneway'} . "\n");
		    debug("$str rtt time = " . $tstats{$str}{'rtt'} . "\n");
		    debug("s = ".$bs{$str}{'S_send'}."; sa = ".$bs{$str}{'S_ack'}."\n");
		}
		
		if (defined($bs{$str}{'M_cryptotime'})) {
		    $tstats{$str}{'cryptotime'} = $bs{$str}{'M_cryptotime'};

		    debug("$str crypto time = " . $tstats{$str}{'cryptotime'} . "\n");
		    ++$cryptotime_count;
		}
		else {
		    $tstats{$str}{'cryptotime'} = -1;
		}
	    }
	}
    }

    # print out compute time stats and recv->send time stats
    # ugh.
    # oneway
    my @ows = ();
    # rtt
    my @rws = ();
    # cpu
    my @cws = ();
    
    for (my $i = 0; $i < 1000; ++$i) {
	$ows[$i] = 0;
	$rws[$i] = 0;
	$cws[$i] = 0;
    }
    foreach my $id (keys(%tstats)) {
	if (defined($tstats{$id}{'oneway'})) {
	    my $slot = int($tstats{$id}{'oneway'} / 1000);
	    ++($ows[$slot]);
	}
	if (defined($tstats{$id}{'rtt'})) {
	    my $slot = int($tstats{$id}{'rtt'} / 1000);
	    ++($rws[$slot]);
	}

	if (defined($tstats{$id}{'cryptotime'})) {
	    my $slot = int($tstats{$id}{'cryptotime'} / 1000);
	    ++($cws[$slot]);
	}

    }
    
    # then, once have times, print out a cdf of them:
    #   1) we're not going for each microsecond because that'd be a 
    #      1000000-line file.  We'll do millisecond granularity.
    #   2) for now, don't include failures?

    my $onewaycdfdatfile = "$outputdir/$basename.onewaycdf.dat";
    open(CDFDAT,">$onewaycdfdatfile") 
	or die "couldn't open $onewaycdfdatfile!";
    my $cdf_counter = 0;
    for (my $i = 0; $i < 1000; ++$i) {
	$cdf_counter += $ows[$i];

	print CDFDAT "$i " . ($cdf_counter/$block_count*100) . "\n";

	if ($cdf_counter == $rtts) {
	    last;
	}
    }
    close(CDFDAT);

    my $rttcdfdatfile = "$outputdir/$basename.rttcdf.dat";
    open(DAT,">$rttcdfdatfile") 
	or die "couldn't open $rttcdfdatfile!";
    $cdf_counter = 0;
    for (my $i = 0; $i < 1000; ++$i) {
	$cdf_counter += $rws[$i];

	print DAT "$i " . ($cdf_counter/$block_count*100) . "\n";

	if ($cdf_counter == $rtts) {
	    last;
	}
    }
    close(DAT);

    my $cpudatfile = "$outputdir/$basename.cpu.dat";
    open(CRYPTODAT,">$cpudatfile") 
	or die "couldn't open $cpudatfile!";
#    my $i = 0;
#    for (my $mi = 0; $mi < $msg_count; ++$mi) {
#	for (my $bi = 0; $bi < $block_count; ++$bi) {
#	    for (my $fi = 0; $fi < $frag_count; ++$fi) {
#		my $str = "m$mi b$bi f$fi";
#		print CRYPTODAT "$i " . $tstats{$str}{'cryptotime'};
#		++$i;
#	    }
#	}
#    }
    $cdf_counter = 0;
    for (my $i = 0; $i < 1000; ++$i) {
	$cdf_counter += $cws[$i];

	print CRYPTODAT "$i " . ($cdf_counter/$block_count*100) . "\n";

	if ($cdf_counter == $cryptotime_count
	    || ($cdf_counter/$block_count*100) == 100) {
	    last;
	}
    }

    close(CRYPTODAT);

    # generate gnuplot src files:
    $file = "$outputdir/$basename.onewaycdf.gnp";
    my $graphfile = "$outputdir/$basename.onewaycdf.eps";
    open(CDFGNP,">$file") 
	or die("couldn't open $file!");
    print CDFGNP "set terminal postscript eps \n";
    print CDFGNP "set output '$graphfile'\n";
    print CDFGNP "set yrange [0:]\n";
    print CDFGNP "set xrange [0:]\n";
    print CDFGNP "set ylabel 'Percent Complete'\n";
    print CDFGNP "set xlabel 'Deadline (ms)'\n";
    print CDFGNP "set key bottom right\n";
    print CDFGNP "plot \\\n";
    print CDFGNP "  '$onewaycdfdatfile' title \"oneway\" with lines 2\n";
#    print CDFGNP "\n";
    close(CDFGNP);

    # run gnuplot   
    system("gnuplot < $file");

    $file = "$outputdir/$basename.rttcdf.gnp";
    $graphfile = "$outputdir/$basename.rttcdf.eps";
    open(CDFGNP,">$file") 
	or die("couldn't open $file!");
    print CDFGNP "set terminal postscript eps \n";
    print CDFGNP "set output '$graphfile'\n";
    print CDFGNP "set yrange [0:]\n";
    print CDFGNP "set xrange [0:]\n";
    print CDFGNP "set ylabel 'Percent Complete'\n";
    print CDFGNP "set xlabel 'Deadline (ms)'\n";
    print CDFGNP "set key bottom right\n";
    print CDFGNP "plot \\\n";
    print CDFGNP "  '$rttcdfdatfile' title \"rtt\" with lines 2\n";
    print CDFGNP "\n";
    close(CDFGNP);

    # run gnuplot   
    system("gnuplot < $file");

    # both oneway and rtt cdfs
    $file = "$outputdir/$basename.bothcdf.gnp";
    my $graphfile = "$outputdir/$basename.bothcdf.eps";
    open(CDFGNP,">$file") 
	or die("couldn't open $file!");
    print CDFGNP "set terminal postscript eps \n";
    print CDFGNP "set output '$graphfile'\n";
    print CDFGNP "set yrange [0:]\n";
    print CDFGNP "set xrange [0:]\n";
    print CDFGNP "set ylabel 'Percent Complete'\n";
    print CDFGNP "set xlabel 'Deadline (ms)'\n";
    print CDFGNP "set key bottom right\n";
    print CDFGNP "plot \\\n";
    print CDFGNP "  '$onewaycdfdatfile' title \"oneway\" with lines 2,\\\n";
    # combining oneway and rtt graphs
    print CDFGNP "  '$rttcdfdatfile' title \"rtt\" with lines 2\n";
#    print CDFGNP "\n";
    close(CDFGNP);

    # run gnuplot   
    system("gnuplot < $file");

    $file = "$outputdir/$basename.cpu.gnp";
    $graphfile = "$outputdir/$basename.cpu.eps";
    open(CDFGNP,">$file") 
	or die("couldn't open $file!");
    print CDFGNP "set terminal postscript eps \n";
    print CDFGNP "set output '$graphfile'\n";
    print CDFGNP "set yrange [0:]\n";
    print CDFGNP "set xrange [0:]\n";
    print CDFGNP "set ylabel 'Percent Complete'\n";
    print CDFGNP "set xlabel 'Deadline (ms)'\n";
    print CDFGNP "set key bottom right\n";
    print CDFGNP "plot \\\n";
    print CDFGNP "  '$cpudatfile' title \"graph\" with lines 2\n";
#    print CDFGNP "\n";
    close(CDFGNP);

    # run gnuplot   
    system("gnuplot < $file");


}



exit 0;
