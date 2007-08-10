# dump2filter.pl

# TODO: This currently does not report losses. Hopefully that doesn't matter.

# Usage: dump2filter.pl <path>
#
# Opens <path>/source.dump <path>/dest1.dump <path>/dest2.dump

$basePath = $ARGV[0];
#$dumpPattern = '^([0-9.]*) IP [0-9.]+ > ([0-9.]+): [A-Za-z. ]+([0-9]+)';
$dumpPattern = '^([0-9.]*) ([0-9.]+) ([0-9]+)$';

sub setupDest
{
    my $fileName = $basePath.'/dest'.$_[0].'.dump';
    my %hash;
    my $file;
    open($file, "<".$fileName)
	or die("Couldn't open file $fileName for reading");
    my @array = <$file>;
    for ($i = 0; $i < @array; ++$i)
    {
	$array[$i] =~ /$dumpPattern/;
	$hash{$3} = $1;
    }
    $array[0] =~ /$dumpPattern/;
    if ($_[0] == 1)
    {
	$dest1Address = $2;
    }
    else
    {
	$dest2Address = $2;
    }
    return %hash;
}

open(SOURCE_IN, "<$basePath/source.dump")
    or die("Couldn't open file $basePath/source.dump for reading");
@source = <SOURCE_IN>;
close(SOURCE_IN);

%dest1 = setupDest(1);
%dest2 = setupDest(2);

open(SOURCE_OUT, ">$basePath/source.filter");
open(DEST1_OUT, ">$basePath/dest1.filter");
open(DEST2_OUT, ">$basePath/dest2.filter");

# Sequence numbers to print out.
$sequence1 = 0;
$sequence2 = 0;
$merged = 0;

for ($i = 0; $i < @source; ++$i)
{
    $sourceLine = $source[$i];
    $sourceLine =~ /$dumpPattern/;
    $sourceTime = $1;
    $flowAddress = $2;
    $flowSequence = $3;

    if ($flowAddress == $dest1Address)
    {
	$destNum = 1;
	$destSequence = $sequence1;
	++$sequence1;
	$destTime = $dest1{$3};
    }
    else
    {
	$destNum = 2;
	$destSequence = $sequence2;
	++$sequence2;
	$destTime = $dest2{$3};
    }
    if (defined($destTime))
    {
	if ($flowAddress == $dest1Address)
	{
	    print DEST1_OUT 'Rcvr: (f'.$destNum.':'.$destSequence.':M'.$merged.')@'.$sourceTime.' arr '.$destTime."\n";
	}
	else
	{
	    print DEST2_OUT 'Rcvr: (f'.$destNum.':'.$destSequence.':M'.$merged.')@'.$sourceTime.' arr '.$destTime."\n";
	}

	print SOURCE_OUT 'Src1: send (f'.$destNum.':'.$destSequence.':M'.$merged.')@'.$sourceTime."\n";
	++$merged;
    }
}
