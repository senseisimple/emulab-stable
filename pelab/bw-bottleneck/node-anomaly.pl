@nodeTotal = ();
@nodeInteresting = ();

for ($i = 0; $i < 1000; ++$i)
{
    push(@nodeTotal, 0);
    push(@nodeAnomaly, 0);
}

@normal = ();
@interesting = ();

while ($line = <STDIN>)
{
    if ($line =~ /Normal: (.*)$/)
    {
	@normal = split(/ /, $1);
    }
    elsif ($line =~ /Interesting: (.*)$/)
    {
	@interesting = split(/ /, $1);
    }
}

for ($i = 0; $i < scalar(@normal); ++$i)
{
    $normal[$i] =~ /([0-9]*)-to-([0-9]*)\.graph/;
    $nodeTotal[$1]++;
    $nodeTotal[$2]++;
}

for ($i = 0; $i < scalar(@interesting); ++$i)
{
    $interesting[$i] =~ /([0-9]*)-to-([0-9]*)\.graph/;
    $nodeTotal[$1]++;
    $nodeInteresting[$1]++;
    $nodeTotal[$2]++;
    $nodeInteresting[$2]++;
}

for ($i = 0; $i < 1000; ++$i)
{
    if ($nodeTotal[$i] > 0)
    {
	print($nodeInteresting[$i]/$nodeTotal[$i] . "\t" . $nodeTotal[$i] . "\t" . $i . "\n");
    }
}

