# delay2dump.pl


if ($#ARGV != 1)
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
    $send1 = $2;
    $send2 = $4;
    $delay1 = $1;
    $delay2 = $3;

    $send1_delay = ($send1 + $delay1)*1000;
    $send2_delay = ($send2 + $delay2)*1000;

    $send1 = $send1*1000;
    $send2 = $send2*1000;

    $send1_sec = $send1/1000000;
    $send1_usec = $send1 % 1000000;

    $send2_sec = $send2/1000000;
    $send2_usec = $send2 % 1000000;

    $send1_delay_sec = $send1_delay/1000000;
    $send1_delay_usec = $send1_delay % 1000000;

    $send2_delay_sec = $send2_delay/1000000;
    $send2_delay_usec = $send2_delay % 1000000;

    if ($send1 ne -9999 && $send2 ne -9999)
    {
        printf (OUT_SOURCE "%d.%d 10.0.0.2 %d\n", $send1_sec,$send1_usec,$sequence);
        printf (OUT_SOURCE "%d.%d 10.0.0.3 %d\n", $send2_sec,$send2_usec,$sequence);
        printf (OUT_DEST1 "%d.%d 10.0.0.2 %d\n",$send1_delay_sec,$send1_delay_usec,$sequence);
        printf (OUT_DEST2 "%d.%d 10.0.0.3 %d\n",$send2_delay_sec,$send2_delay_usec,$sequence);
    }

    $sequence++;
}
