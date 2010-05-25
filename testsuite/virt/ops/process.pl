#!/usr/bin/perl
#
# EMULAB-COPYRIGHT
# Copyright (c) 2009 University of Utah and the Flux Group.
# All rights reserved.
#

if (scalar(@ARGV) != 1)
{
    print STDERR "Usage: process.pl <result-path>\n";
    exit(1);
}

$resultPrefix = $ARGV[0];

$IDLE_FIELD = 15;
$SYSTEM_FIELD = 14;
$FREE_FIELD = 4;
$INTERRUPT_FIELD = 11;

sub getBandwidthList
{
    my $prefix = shift(@_);
    my $pairCount = shift(@_);
    my @result = ();
    my $i = 0;
    for ($i = 1; $i <= $pairCount; ++$i)
    {
	my $file = "$prefix/client-$i/local/logs/client-$i-agent.out";
	my $line = `tail -n 1 $file`;
	my $current = 0;
	if ($line =~ /([0-9.]+) Gbits\/sec/)
	{
	    $current = $1 * 1024;
	}
	elsif ($line =~ /([0-9.]+) Mbits\/sec/)
	{
	    $current = $1;
	}
	elsif ($line =~ /([0-9.]+) Kbits\/sec/)
	{
	    $current = $1 / 1024;
	}
	elsif ($line =~ /([0-9.]+) bits\/sec/)
	{
	    $current = $1 / 1024 / 1024;
	}
	push(@result, $current);
    }
    return @result;
}

sub getSlabList
{
    my $prefix = shift(@_);
    my $systemString = `grep skbuff_fclone_cache $prefix/vhost-0/local/logs/vmstat-slab.out | sed 's/  / /g' | sed 's/  / /g' | sed 's/  / /g' | sed 's/  / /g' | cut -d ' ' -f 3 | tail -n +10`;
    my @lines = split(/\n/, $systemString);
    my @result = ();
    my $i = 0;
    for ($i = 0; $i < scalar(@lines); ++$i)
    {
	push(@result, $lines[$i] * 384);
    }
    return @result;
}

sub getCpuList
{
    my $prefix = shift(@_);
    my $field = shift(@_);
    my $systemString = `tail -n +7 $prefix/vhost-0/local/logs/vmstat-cpu.out | sed 's/  / /g' | sed 's/  / /g' | sed 's/  / /g' | sed 's/^ //' | cut -d ' ' -f $field`;
    my @systems = split(/\n/, $systemString);
    my @result = ();
    my $i = 0;
    for ($i = 0; $i < scalar(@systems); ++$i)
    {
	push(@result, $systems[$i]);
    }
    return @result;
}

sub scaleList
{
    my $factor = shift(@_);
    my $i = 0;
    my @result = ();
    for ($i = 0; $i < scalar(@_); ++$i)
    {
	push(@result, $factor * @_[$i]);
    }
    return @result;
}

sub invertBoundList
{
    my $bound = shift(@_);
    my $i = 0;
    my @result = ();
    for ($i = 0; $i < scalar(@_); ++$i)
    {
	push(@result, $bound - @_[$i]);
    }
    return @result;
}

sub totalList
{
    my $i = 0;
    my $result = 0;
    for ($i = 0; $i < scalar(@_); ++$i)
    {
	$result += @_[$i];
    }
    return $result;
}

sub meanList
{
    my $result = -999;
    my $total = totalList(@_);
    if (scalar(@_) > 0)
    {
	$result = $total / scalar(@_);
    }
    return $result;
}

sub stderrList
{
    my @data = @_;
    my $i = 0;
    my $n = scalar(@data);
    if ($n < 2)
    {
	return 0;
    }
    my $mean = meanList(@data);

    my $total = 0;
    for ($i = 0; $i < $n; ++$i)
    {
	my $diff = $data[$i] - $mean;
	$total += $diff * $diff;
    }
    my $stddev = sqrt($total / ($n-1));
    my $stderr = $stddev / sqrt($n);
    return $stderr;
}

sub lowerBound
{
    my $mean = shift(@_);
    my $stderr = shift(@_);
    my $lower = $mean - $stderr * 1.96;
    return $lower;
}

sub upperBound
{
    my $mean = shift(@_);
    my $stderr = shift(@_);
    my $upper = $mean + $stderr * 1.96;
    return $upper;
}

sub errorBarLine
{
    my $count = shift(@_);
    my @data = @_;
    my $mean = meanList(@data);
    my $stderr = stderrList(@data);
    my $lower = lowerBound($mean, $stderr);
    my $upper = upperBound($mean, $stderr);
    return "$count $mean $lower $upper\n";
}

system("mkdir -p $resultPrefix/graph");
system("mkdir -p $resultPrefix/plot");

my @pairList = (1, 2, 5, 10, 15, 20, 25, 30, 35);
my @bwList = (500, 1000, 2000, 5000, 10000, 20000, 50000, 100000, 200000, "unlimited");
my $delay = 0;


sub printPlot
{
    my $nick = shift(@_);
    my $prefix = shift(@_);
    my $output = shift(@_);
    my @bwList = @_;
    open PLOT_FILE, ">$resultPrefix/plot/$nick-plot";
    print PLOT_FILE "$prefix\n";
    print PLOT_FILE "set term postscript enhanced\n";
    print PLOT_FILE "set output\"$resultPrefix/$output.ps\"\n";
    print PLOT_FILE "plot ";
    my $i = 0;
    for ($i = 0; $i < scalar(@bwList); ++$i)
    {
	if ($i != 0)
	{
	    print PLOT_FILE ", ";
	}
	my $bw = $bwList[$i];
	if ($bw ne "unlimited")
	{
	    $bw = $bw / 1000;
	}
	print PLOT_FILE "\"$resultPrefix/graph/$nick-" . $bwList[$i]
	    . "-0.graph\" with linespoints title \"$bw Mbps, 0 ms\"";
    }

    print PLOT_FILE ";\n";
    close PLOT_FILE;
}

sub runPlot
{
    my $nick = shift(@_);
    system("gnuplot $resultPrefix/plot/$nick-plot");
}

my $totalPlot = <<'PLOTEND';
set title "Aggregate Bandwidth";
set xlabel "# of Pairs";
set ylabel "Aggregate Bandwidth (Mbps)";
set yrange [0:];
PLOTEND

printPlot("total", $totalPlot, "openvz-total-bandwidth", @bwList);

my $meanPlot = <<'PLOTEND';
set title "Mean Bandwidth";
set xlabel "# of Pairs";
set ylabel "Mean Bandwidth (Mbps)";
set yrange [0:];
PLOTEND

printPlot("mean", $meanPlot, "openvz-mean-bandwidth", @bwList);

my $cpuPlot = <<'PLOTEND';
set title "Mean CPU Usage (total)";
set xlabel "# of Pairs";
set ylabel "Mean CPU usage (total) (%)";
set yrange [0:100];
PLOTEND

printPlot("cpu", $cpuPlot, "openvz-cpu", @bwList);

my $systemPlot = <<'PLOTEND';
set title "Mean CPU Usage (system)";
set xlabel "# of Pairs";
set ylabel "Mean CPU usage (system) (%)";
set yrange [0:100];
PLOTEND

printPlot("system", $systemPlot, "openvz-system", @bwList);

my $memoryPlot = <<'PLOTEND';
set title "Mean Free Page Count";
set xlabel "# of Pairs";
set ylabel "Mean Free Page Count (thousands)";
set yrange [0:];
PLOTEND

printPlot("memory", $memoryPlot, "openvz-memory", @bwList);

my $interruptPlot = <<'PLOTEND';
set title "Mean Interrupt Rate";
set xlabel "# of Pairs";
set ylabel "Mean Interrupt Rate (per second)";
set yrange [:];
set key left top;
PLOTEND

printPlot("interrupt", $interruptPlot, "openvz-interrupt", @bwList);

my $slabPlot = <<'PLOTEND';
set title "Mean skbuff-fclone-cache Slab Size"
set xlabel "# of Pairs"
set ylabel "Mean Slab Size (KB)"
set yrange [:];
set key left top;
PLOTEND

printPlot("slab", $slabPlot, "openvz-slab", @bwList);

my $i = 0;
my $j = 0;
for ($i = 0; $i < scalar(@bwList); ++$i)
{
    open TOTAL_OUT, ">$resultPrefix/graph/total-".$bwList[$i]."-0.graph";
    open MEAN_OUT, ">$resultPrefix/graph/mean-".$bwList[$i]."-0.graph";
    open CPU_OUT, ">$resultPrefix/graph/cpu-".$bwList[$i]."-0.graph";
    open SYSTEM_OUT, ">$resultPrefix/graph/system-".$bwList[$i]."-0.graph";
    open MEMORY_OUT, ">$resultPrefix/graph/memory-".$bwList[$i]."-0.graph";
    open INTERRUPT_OUT, ">$resultPrefix/graph/interrupt-".$bwList[$i]."-0.graph";
    open SLAB_OUT, ">$resultPrefix/graph/slab-".$bwList[$i]."-0.graph";
    for ($j = 0; $j < scalar(@pairList); ++$j)
    {
	my $count = $pairList[$j];
	my $prefix = "$resultPrefix/output/" . $pairList[$j] . "-"
	    . $bwList[$i] . "-$delay/logs";
	if (-e $prefix)
	{
	  my @bwResults = getBandwidthList($prefix, $count);
	  my $total = totalList(@bwResults);
	  print TOTAL_OUT "$count $total\n";
	  print MEAN_OUT errorBarLine($count, @bwResults);
	  
	  my @idleResults = getCpuList($prefix, $IDLE_FIELD);
	  my @cpuResults = invertBoundList(100, @idleResults);
	  print CPU_OUT errorBarLine($count, @cpuResults);

	  my @systemResults = getCpuList($prefix, $SYSTEM_FIELD);
	  print SYSTEM_OUT errorBarLine($count, @systemResults);

	  my @memoryResults = getCpuList($prefix, $FREE_FIELD);
	  my @memoryThousands = scaleList(1/1000.0, @memoryResults);
	  print MEMORY_OUT errorBarLine($count, @memoryThousands);

	  my @interruptResults = getCpuList($prefix, $INTERRUPT_FIELD);
	  print INTERRUPT_OUT errorBarLine($count, @interruptResults);

	  my @slabResults = getSlabList($prefix);
	  my @slabK = scaleList(1/1024.0, @slabResults);
	  print SLAB_OUT errorBarLine($count, @slabK);
      }
    }
    close TOTAL_OUT;
    close MEAN_OUT;
    close CPU_OUT;
    close SYSTEM_OUT;
    close MEMORY_OUT;
    close INTERRUPT_OUT;
    close SLAB_OUT;
}

runPlot("total");
runPlot("mean");
runPlot("cpu");
runPlot("system");
runPlot("memory");
runPlot("interrupt");
runPlot("slab");
