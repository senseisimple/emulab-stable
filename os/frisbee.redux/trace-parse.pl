#!/usr/bin/perl
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2011 University of Utah and the Flux Group.
# All rights reserved.
#

use Getopt::Std;

sub usage()
{
    exit(1);
}
my $optlist = "pi:cT:j";

my $plotem = 0;
my $plotinterval = 500000;	# us between data points
my $nextplot = -1;
my ($netfd, $dcfd, $diskfd);
my $cumulative = 0;
my $dojpeg = 0;
my $ptitle;

my $isclient;
my $netstart;
my ($gotevents, $totevents, $starttime, $tracelevel);
my $laststamp;
my $lineno = 0;

my ($c_blocks, $c_blocksize);
my ($c_firstrecv, $c_lastrecv, $c_recvbytes);
my ($c_firstdecomp, $c_lastdecomp, $c_decompbytes);
my ($c_firstwrite, $c_lastwrite, $c_writebytes);
my ($c_prevstamp, $c_prevrbytes, $c_prevdbytes, $c_prevwbytes);
my ($c_fsyncstart, $c_fsyncend);

#
# Parse command arguments.
#
%options = ();
if (! getopts($optlist, \%options)) {
    usage();
}
if (defined($options{"c"})) {
    $cumulative = 1;
    $plotinterval = 100000;
}
if (defined($options{"h"})) {
    usage();
}
if (defined($options{"i"})) {
    $plotinterval = int($options{"i"});
}
if (defined($options{"j"})) {
    $dojpeg = 1;
}
if (defined($options{"p"})) {
    $plotem = 1;
}
if (defined($options{"T"})) {
    $ptitle = $options{"T"};
}

while (@ARGV) {
    my $file = shift @ARGV;
    processfile($file);
}

#
# Round up a time in seconds to the next plot interval value.
#
sub
plotroundup($)
{
    my $val = shift;

    $val = int($val * 1000000) + $plotinterval;
    $val = int($val / $plotinterval) * $plotinterval;
    return $val / 1000000.0;
}

sub
processfile($)
{
    my ($file) = @_;
    my ($netfile, $dcfile, $diskfile);

    if (!open(FD, "$file")) {
	printf STDERR "$file: $!\n";
	return 1;
    }

    if ($plotem) {
	$netfile = "$ptitle-net.data";
	$dcfile = "$ptitle-dcomp.data";
	$diskfile = "$ptitle-disk.data";
	if (!open($netfd, ">$netfile") || !open($dcfd, ">$dcfile") ||
	    !open($diskfd, ">$diskfile")) {
	    printf STDERR "WARNING: can't open a plot file, not generating\n";
	    $plotem = 0;
	} else {
	    $nextplot = 0;
	}
    }

    $lineno = 0;
    while (<FD>) {
	chomp;
	$lineno++;

	# header
	if (/(\d+) of (\d+) events, start=(\d+\.\d+), level=(\d+):/) {
	    if (defined($gotevents)) {
		print STDERR "$lineno: two event lines; tracefile has mulitple runs?\n";
		exit(1);
	    }
	    $gotevents = $1;
	    $totevents = $2;
	    $starttime = $3;
	    $tracelevel = $4;

	    if ($gotevents != $totevents) {
		print STDERR "$lineno: WARNING: only $gotevents of $totevents events recorded; all bets off!\n";
	    }
	    next;
	}

	# client event record
	if (/\s+\+(\d+\.\d+): C: (\d+\.\d+\.\d+\.\d+): (.*)/) {
	    $laststamp = $1;
	    processclient($1, $2, $3);
	    next;
	}

	# server event record
	if (/\s+\+(\d+\.\d+): S: (.*)/) {
	    $laststamp = $1;
	    processserver($1, $2);
	    next;
	}

	print STDERR "Record in unrecognized format:\n    $_\n";
    }
    finishclient($laststamp);
    finishserver($laststamp);

    close(FD);

    if ($plotem) {
	#
	# Round end of net/decompress activity up to interval boundaries
	# so we can show them on the graph
	#
	my $nstart = plotroundup($c_firstrecv);
	my $nstop = plotroundup($c_lastrecv);
	my $dstart = plotroundup($c_firstdecomp);
	my $dstop = plotroundup($c_lastdecomp);
	my $wstart = plotroundup($c_firstwrite);
	my $wstop = plotroundup($c_lastwrite);

	my $gnpfile = "$ptitle.gnp";
	if (!open(PD, ">$gnpfile")) {
	    printf STDERR "$gnpfile: WARNING: cannot open ($!), not generating\n";
	} else {
	    # XXX
	    my $ylim = $cumulative ? 40 : 100;

	    if ($dojpeg) {
		print PD
		    "set terminal jpeg medium\n",
		    "set output '$ptitle.jpg'\n";
	    } else {
		print PD
		    "set terminal postscript color\n",
		    "set output '$ptitle.ps'\n";
	    }
	    print PD
		"set yrange [0:$ylim]\n",
		"set xlabel 'Elapsed time (sec)'\n",
		"set ylabel 'Throughput (MB/sec)'\n",
		"set key top left\n",
		"set title '$ptitle'\n",
		"set arrow from $nstart, graph 0 to $nstart, graph 1 nohead lt 1\n",
		"set arrow from $nstop, graph 0 to $nstop, graph 1 nohead lt 1\n",
		"set arrow from $dstart, graph 0 to $dstart, graph 1 nohead lt 2\n",
		"set arrow from $dstop, graph 0 to $dstop, graph 1 nohead lt 2\n",
		"set arrow from $wstart, graph 0 to $wstart, graph 1 nohead lt 3\n",
		"set arrow from $wstop, graph 0 to $wstop, graph 1 nohead lt 3\n",
		"plot '$netfile' title 'net read' with lines, ",
		"'$dcfile' title 'decompress' with lines, ",
		"'$diskfile' title 'disk write' with lines\n";
	    close(PD);
	}
	close($netfd);
	close($dcfd);
	close($diskfd);
    }
}

sub
processclient($$$)
{
    my ($stamp, $ip, $msg) = @_;

    if (!defined($ptitle)) {
	$ptitle = $ip;
    }

    if ($nextplot >= 0) {
	while (($stamp * 1000000) > $nextplot) {
	    plotclient($nextplot);
	    $nextplot += $plotinterval;
	}
    }

    if ($msg =~ /^got JOIN reply, chunksize=(\d+), blocksize=(\d+), imagebytes=(\d+)/) {
	$c_blocks = int($3 / $2);
	$c_blocksize = $2;
    }

    if ($msg =~ /^send REQUEST, (\d+)\[(\d+)-(\d+)\]/) {
	;
    }

    if ($msg =~ /^got block, wait=(\d+\.\d+), (\d+) good blocks recv \((\d+) total\)/) {
	if ($tracelevel >= 4) {
	    $c_firstrecv = $stamp
		if (!defined($c_firstrecv));
	    $c_lastrecv = $stamp;
	}
	$c_recvbytes = $2 * $c_blocksize;
    }

    if ($msg =~ /^start chunk (\d+), block (\d+), (\d+) chunks in progress \(goodblks=(\d+)\)/) {
	if ($tracelevel < 4) {
	    $c_firstrecv = $stamp
		if (!defined($c_firstrecv));
	    $c_lastrecv = $stamp;
	}
	$c_recvbytes = $4 * $c_blocksize;
    }

    if ($msg =~ /^end chunk (\d+), block (\d+), (\d+) chunks in progress \(goodblks=(\d+)\)/) {
	$c_recvbytes = $4 * $c_blocksize;	
    }
    if ($msg =~ /^decompressing chunk (\d+), idle=(\d+), \(dblock=(\d+), widle=(\d+)\)/) {
	$c_firstdecomp = $stamp
	    if (!defined($c_firstdecomp));
    }
    if ($msg =~ /^chunk (\d+) \(\d+ bytes\) decompressed, (\d+) left/) {
	$c_lastdecomp = $stamp;
    }
    if ($msg =~ /^decompressed (\d+) bytes total/) {
	$c_decompbytes = $1;
    }

    if ($msg =~ /^writer STARTED, (\d+) bytes written/) {
	$c_firstwrite = $stamp
	    if (!defined($c_firstwrite));
	$c_lastwrite = $stamp;
	$c_writebytes = $1;
    }
    if ($msg =~ /^writer RUNNING, (\d+) bytes written/) {
	$c_lastwrite = $stamp;
	$c_writebytes = $1;
    }
    if ($msg =~ /^writer IDLE, (\d+) bytes written/) {
	$c_lastwrite = $stamp;
	$c_writebytes = $1;
    }
    if ($msg =~ /^fsync START/) {
	$c_fsyncstart = $stamp;
    }
    if ($msg =~ /^fsync END/) {
	$c_fsyncend = $stamp;
    }
}

sub
nz($)
{
    my ($val) = @_;
    return ($val == 0 ? 1 : $val);
}

sub
plotclient($)
{
    my ($stamp) = @_;
    my ($rbytes, $dbytes, $wbytes, $ilen);

    if ($cumulative) {
	$rbytes = $c_recvbytes;
	$dbytes = $c_decompbytes;
	$wbytes = $c_writebytes;
	$ilen = $stamp / 1000000;

    } else {
	$rbytes = $c_recvbytes - $c_prevrbytes;
	$dbytes = $c_decompbytes - $c_prevdbytes;
	$wbytes = $c_writebytes - $c_prevwbytes;
	$ilen = $plotinterval / 1000000;
    }
    $c_prevrbytes = $c_recvbytes;
    $c_prevdbytes = $c_decompbytes;
    $c_prevwbytes = $c_writebytes;

    printf({$netfd} "%.3f: %.6f\n",
	   $stamp / 1000000, $rbytes / nz($ilen) / 1000000);
    printf({$dcfd} "%.3f: %.6f\n",
	   $stamp / 1000000, $dbytes / nz($ilen) / 1000000);
    printf({$diskfd} "%.3f: %.6f\n",
	   $stamp / 1000000, $wbytes / nz($ilen) / 1000000);
}

sub
finishclient($)
{
    my ($stamp) = @_;
    my $etime;

    if ($c_decompbytes == 0) {
	print STDERR "WARNING: no decompression count, try -tt\n";
    }

    if ($nextplot >= 0) {
	while (($stamp * 1000000) > $nextplot) {
	    plotclient($nextplot);
	    $nextplot += $plotinterval;
	}
    }
    $etime = $c_lastrecv - $c_firstrecv;
    my $rbytes = $c_blocks * $c_blocksize;
    printf("Receive:    %.2f MB/sec: %d bytes (%d %dK blocks) in %.3f sec (%.3f to %.3f)\n",
	   $rbytes / nz($etime) / 1000000.0,
	   $rbytes, $c_blocks, $c_blocksize/1024,
	   $etime, $c_firstrecv, $c_lastrecv);

    $etime = $c_lastdecomp - $c_firstdecomp;
    printf("Decompress: %.2f MB/sec: %d->%d bytes in %.3f sec (%.3f to %.3f)\n",
	   $c_decompbytes / nz($etime) / 1000000.0,
	   $rbytes, $c_decompbytes, $etime, $c_firstdecomp, $c_lastdecomp);

    if (($c_fsyncend - $c_fsyncstart) * 1000 >= 1) {
	$c_lastwrite = $c_fsyncend;
    }
    $etime = $c_lastwrite - $c_firstwrite;
    printf("Write:      %.2f MB/sec: %d bytes in %.3f sec (%.3f to %.3f)\n",
	   $c_writebytes / nz($etime) / 1000000.0,
	   $c_writebytes, $etime, $c_firstwrite, $c_lastwrite);
}

sub
processserver($$)
{
    my ($stamp, $msg) = @_;
    my $clientip = undef;

    if ($msg =~ /(\d+\.\d+\.\d+\.\d+): (.*)/) {
	$clientip = $1;
	$msg = $2;
    }
}

sub
finishserver($)
{
    my ($stamp) = @_;
}

exit(0);
