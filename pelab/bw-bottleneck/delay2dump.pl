# delay2dump.pl


if ($ARGV != 2)
{
    die ("Usage: delay2dump.pl <in-file> <out-path>");
}

open (IN, "<".$ARGV[0]) or die("Cannot open ".$ARGV[0]);
open (OUT_SOURCE, ">".$ARGV[1]."/source.dump") or die("Cannot open source");
open (OUT_DEST1, ">".$ARGV[1]."/dest1.dump") or die("Cannot open dest 1");
open (OUT_DEST2, ">".$ARGV[1]."/dest2.dump") or die("Cannot open dest 2");

$sequence = 0;

while ($line = <IN>)
{
    $line =~ /^([0-9.]+) ([0-9.]+) ([0-9.]+) ([0-9.]+)$/;
    $send1 = $1;
    $send2 = $2;
    $delay1 = $3;
    $delay2 = $4;

    if ($send1 ne -9999 && $send2 ne -9999)
    {
	print (OUT_SOURCE, $send1." 10.0.0.2 ".$sequence);
	print (OUT_SOURCE, $send2." 10.0.0.3 ".$sequence);
	print (OUT_DEST1, ($send1 + $delay1)." 10.0.0.1 ".$sequence);
	print (OUT_DEST2, ($send2 + $delay2)." 10.0.0.1 ".$sequence);
    }

    $sequence++;
}
