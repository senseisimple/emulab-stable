$dest = $ARGV[0];
$port = $ARGV[1];
$duration = $ARGV[2];

for ($i = 1; $i < 11; ++$i)
{
    if ($i > 1)
    {
      $rateLine = `/bw-bottleneck/iperf -c $dest -p $port -w 256k -i 10 -t $duration -P $i -f m | grep SUM | tail -n 1`;
    }
    else
    {
      $rateLine = `/bw-bottleneck/iperf -c $dest -p $port -w 256k -i 10 -t $duration -P $i -f m | tail -n 1`;
    }

    $rateLine =~ /([0-9.]+) Mbits\/sec$/;
    $rate = $1;
    print "$i $rate \n";
}
