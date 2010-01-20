%seen = ();

while ($line = <STDIN>)
{
    if ($line =~ /[0-9a-z.]+ > [0-9a-z.]+: [A-Z.]+ ([0-9]+):[0-9]+\([0-9]+\) win ([0-9]+) .*/)
    {
	$sequence = $1;
	$winSize = $2;
	if ($winSize == 0)
	{
	    print "Receive Window Size is 0\n";
	}
	if (exists($seen{$sequence}))
	{
	    print "Retransmit detected\n";
	}
	$seen{$sequence} = 1;
    }
}
