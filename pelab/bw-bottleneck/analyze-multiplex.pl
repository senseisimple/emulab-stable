# Usage: analyze-multiplex.pl [filename [...]]

%nodeToPlab = ();
%plabToCap = ();
%plabToCapExempt = ();
%hostToPlab = ();
%plabToSite = ();

sub readNodeMap
{
    open NODE_MAP, "<"."ltpmap";
    my $line;
    while ($line = <NODE_MAP>)
    {
	if ($line =~ /H node-([0-9]+) [^ ]+ (plab[0-9]+)/)
	{
	    $nodeToPlab{$1} = $2;
	}
    }
}

sub readPlabMap
{
    open PLAB_MAP, "<"."plab-map.txt";
    my $line;
    while ($line = <PLAB_MAP>)
    {
	if ($line =~ /(plab[0-9]+)\s+([a-zA-Z0-9.\-]+)\s+([a-zA-Z0-9\-]+)\s+([0-9]*)/)
	{
	    if ($4 == "")
	    {
		$plabToCap{$1} = 0;
	    }
	    else
	    {
		$plabToCap{$1} = $4;
	    }
	    $plabToSite{$1} = $3;
	    $hostToPlab{$2} = $1;
	}
    }
}

sub readExempt
{
    open EXEMPT, "<"."i2.nodes";
    my $line;
    while ($line = <PLAB_EXEMPT>)
    {
	$line =~ /\s*([a-zA-Z0-9.\-]+)\s*/;
	my $plab = $hostToPlab{$1};
	$plabToCapExempt{$plab} = 1;
    }
}

@bandwidths = ();

sub regressSlope
{
    my $begin = shift(@_);
    my $end = shift(@_);
    my $x_i = 0.0;
    my $y_i = 0.0;
    my $numA = 0.0;
    my $numB = 0.0;
    my $numC = 0.0;
    my $denomA = 0.0;
    my $denomB = 0.0;
    my $denomC = 0.0;
    for (my $i = $begin; $i < $end && $i < scalar(@bandwidths); ++$i)
    {
	$x_i = $i + 1;
	$y_i = $bandwidths[$i];
	$numA += $x_i * $y_i;
	$numB += $x_i;
	$numC += $y_i;
	$denomA += $x_i * $x_i;
	$denomB += $x_i;
	$denomC += $x_i;
    }
    my $numD = $end - $begin;
    my $denomD = $end - $begin;
    my $num = ($numA * $numD) - ($numB * $numC);
    my $denom = ($denomA * $denomD) - ($denomB * $denomC);
    my $slope = 0;
    if (abs($denom) > 0.0001)
    {
	$slope = $num/$denom;
    }
#    print("Result: $num / $denom\n");
    return $slope;
}

sub regressIntercept
{
    my $begin = shift(@_);
    my $end = shift(@_);
    my $x_i = 0.0;
    my $y_i = 0.0;
    my $numA = 0.0;
    my $numB = 0.0;
    my $numC = 0.0;
    my $numD = 0.0;
    my $denomA = 0.0;
    my $denomB = 0.0;
    my $denomC = 0.0;
    for (my $i = $begin; $i < $end && $i < scalar(@bandwidths); ++$i)
    {
	$x_i = $i + 1;
	$y_i = $bandwidths[$i];
	$numA += $x_i * $x_i;
	$numB += $x_i * $y_i;
	$numC += $x_i;
	$numD += $y_i;
	$denomA += $x_i * $x_i;
	$denomB += $x_i;
	$denomC += $x_i;
    }
    my $denomD = $end - $begin;
    my $num = ($numA * $numD) - ($numB * $numC);
    my $denom = ($denomA * $denomD) - ($denomB * $denomC);
    my $intercept = 0;
    if (abs($denom) > 0.0001)
    {
	$intercept = $num/$denom;
    }
#    print("Result: $num / $denom\n");
    return $intercept;
}

sub regressError
{
    my $begin = shift(@_);
    my $end = shift(@_);
    my $slope = shift(@_);
    my $intercept = shift(@_);
    my $deviation = 0.0;
    for (my $i = $begin; $i < $end && $i < scalar(@bandwidths); ++$i)
    {
	$lineY = $slope * ($i+1) + $intercept;
	$deviation += ($bandwidths[$i] - $lineY) * ($bandwidths[$i] - $lineY);
    }
    my $mean = 0.0;
    for (my $i = $begin; $i < $end && $i < scalar(@bandwidths); ++$i)
    {
	$mean += $bandwidths[$i];
    }
    $mean = $mean / ($end - $begin);
    # ssm => sum of squares of difference with mean
    my $ssm = 0.0;
    for (my $i = $begin; $i < $end && $i < scalar(@bandwidths); ++$i)
    {
	$ssm += ($bandwidths[$i] - $mean) * ($bandwidths[$i] - $mean);
    }
    if ($ssm == 0)
    {
	return -9999999999;
    }
    else
    {
#	print "dev: " . $deviation . ", ssm: " . $ssm . "\n";
	return 1 - ($deviation / $ssm);
#	return ($ssm - $deviation) / $ssm;
    }
}

readNodeMap();
readPlabMap();
readExempt();

@normal = ();
@interesting = ();
@problem = ();

$counter = 0;

for ($arg = 0; $arg < scalar(@ARGV); ++$arg)
{
    @bandwidths = ();
    open FILE, "<".$ARGV[$arg];

    $good = 1;
    while ($line = <FILE>)
    {
	if ($line =~ /([0-9.]+) ([0-9.]+) /)
	{
	    $x = $1;
	    $y = $2;
	    push(@bandwidths, $y);
	}
	else
	{
	    $good = 0;
	}
    }
    $size = scalar(@bandwidths);

    if ($good == 1 && $size == 10)
    {
    
	$bestLeftSlope = 0;
	$bestRightSlope = 0;
	$bestDivider = 0;
	$bestError = 0;
	for ($divider = 1; $divider < 9; ++$divider)
	{
	    $leftSlope = regressSlope(0, $divider + 1);
	    $leftIntercept = regressIntercept(0, $divider + 1);
	    $rightSlope = regressSlope($divider, $size);
	    $rightIntercept = regressIntercept($divider, $size);
	    
	    $error = (regressError(0, $divider + 1, $leftSlope, $leftIntercept)
		+ regressError($divider, $size, $rightSlope, $rightIntercept))/2;
	    
	    if ($error > $bestError)
	    {
		$bestLeftSlope = $leftSlope;
		$bestRightSlope = $rightSlope;
		$bestDivider = $divider;
		$bestError = $error;
	    }
	}
	
	# Sync up with the # of streams where 1 stream is at position 0.
	$bestDivider += 1;

#	print ($bestError . "\n");
#	++$counter;

# Initial slope positive flat or straight
# straight normalized in terms of bandwidth
# Accuracy in terms of percentage?
# Percent increase in y value?
# should increase by k per flow while window size limited
# Two lines, each of which are increasing decreasing or flat. Divide into 9 buckets.
# 20 node experiment
# keep tcpdump
# check in tonight
# ping

# Find ones where the max value == cap
# Find ones where the max value > cap

	if ($bestError < 0.7
	    || (abs($bestLeftSlope - $bestRightSlope) > 1
		&& $bestLeftSlope < 1
		&& ($bestRightSlope > 0.5 || $bestRightSlope < -0.5)))
	{
#	    print("---\n");
#	    print($ARGV[$arg]."\n");
#	    print("---\n");
#	    print("Error: $bestError\n");
#	    print("Left-Slope: $bestLeftSlope\n");
#	    print("Right-Slope: $bestRightSlope\n");
#	    print("Divider: $bestDivider\n\n");
	    push(@interesting, $ARGV[$arg]);
	}
	else
	{
	    if (abs($bestLeftSlope - $bestRightSlope) <= 1)
	    {
		print("Straight:\t");
	    }
	    else
	    {
		print("Plateau:\t");
	    }
	    $ARGV[$arg] =~ /([0-9]+)-to-([0-9]+)/;
	    $sourcePlab = $nodeToPlab{$1};
	    $destPlab = $nodeToPlab{$2};
	    if ($plabToCap{$sourcePlab} > 0
		&& (! exists($plabToCapExempt{$sourcePlab})
		    || ! exists($plabToCapExempt{$destPlab})))
	    {
		print("Capped\t");
	    }
	    else
	    {
		print("Free\t");
	    }
	    printf("%0.2f %0.2f %0.2f ", $bestError, $leftSlope, $rightSlope);
	    print($ARGV[$arg]."\n");
	    push(@normal, $ARGV[$arg]);
	}
    }
    else
    {
	push(@problem, $ARGV[$arg]);
    }
}

#print("Normal Count: ".scalar(@normal)."\n");
#print("Normal: ".join(" ",@normal)."\n\n");
#print("Interesting Count: ".scalar(@interesting)."\n");
#print("Interesting: ".join(" ",@interesting)."\n\n");
#print("Problem Count: ".scalar(@problem)."\n");
#print("Problem: ".join(" ",@problem)."\n");
