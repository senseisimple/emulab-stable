$number = $ARGV[0];
$count = $ARGV[1];

for ($i = 1; $i <= $count; ++$i)
{
    if ($i != $number)
    {
	system("/bw-bottleneck/iperf -c node-$i.multiplex-small.tbres.emulab.net -t 30 -f m -w 256k > /local/logs/$number-to-$i.result &");
    }
}
