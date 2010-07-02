$total = 7;

sub sendStream
{
    my $streamCount = shift(@_);
    my $dest = shift(@_);
    my $out = shift(@_);
    my $command = "cd /proj/tbres/duerig/testbed/pelab/monitor;".
	" sh instrument.sh iperf -c 10.0.0.$dest -w 256k -P $streamCount -t 30 -f m > $out &";
    print STDERR $command."\n";
    system($command);
}

for ($i = 0; $i < $total; ++$i)
{
    for ($j = 0; $j < $total - $i; ++$j)
    {
	for ($k = 0; $k < $total - $i - $j; ++$k)
	{
	    $l = $total - $i - $j - $k - 1;
	    if ($i > 0)
	    {
		$filename = "/proj/tbres/duerig/multiplex/bottle-all/$i-$j-$k-$l-2.txt";
		sendStream($i, 2, $filename);
		print $filename."\n";
	    }
	    if ($j > 0)
	    {
		$filename = "/proj/tbres/duerig/multiplex/bottle-all/$i-$j-$k-$l-3.txt";
		sendStream($j, 3, $filename);
		print $filename."\n";
	    }
	    if ($k > 0)
	    {
		$filename = "/proj/tbres/duerig/multiplex/bottle-all/$i-$j-$k-$l-4.txt";
		sendStream($k, 4, $filename);
		print $filename."\n";
	    }
	    if ($l > 0)
	    {
		$filename = "/proj/tbres/duerig/multiplex/bottle-all/$i-$j-$k-$l-5.txt";
		sendStream($l, 5, $filename);
		print $filename."\n";
	    }
	    sleep(40);
	}
    }
}
