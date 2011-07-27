

# Usage: run-test.sh <project>/<experiment> <link-name> <node1-name> <node2-name>

$exp = $ARGV[0];
$link = $ARGV[1];
$node1 = $ARGV[2];
$node2 = $ARGV[3];

sub sendEvent
{
    my $source = shift @_;
    my $params = shift @_;
    my $command = "/usr/testbed/bin/tevc -e $exp now $source $params";
    print $command . "\n";
    system($command);
}

sub check
{
    my $testname = shift @_;
    sleep(5);
    my $command;
    $command = "ping -c 10 $node2-$link > test/$testname.ping.$link";
    print $command . "\n";
    system($command);
    $command = "/proj/delay-agent/duerig/iperf -c $node2-$link -w 256k -t 60 > test/$testname.iperf.$link";
    print $command . "\n\n";
    system($command);
}

sendEvent("$link", "UP");
sendEvent($link, "MODIFY DELAY=30");
sendEvent($link, "MODIFY BANDWIDTH=1000");
sendEvent("$link-$node1", "MODIFY DELAY=10");
check("check1");

sendEvent("$link", "MODIFY DELAY=20 BANDWIDTH=500");
check("check2");

sendEvent("$link", "DOWN");
check("check3");

sendEvent("$link-$node2", "MODIFY DELAY=50");
sendEvent("$link-$node1", "MODIFY BANDWIDTH=6000");
sendEvent("$link", "UP");
check("check4");
