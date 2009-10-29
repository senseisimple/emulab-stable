my $pairs = $ARGV[0];

my $count = $pairs * 2;

system("sudo killall delay-agent");
my $i = 0;
for ($i = 1; $i <= $count; ++$i)
{
    system("sudo cp /proj/tbres/duerig/delay-agent/src/testbed/event/delay-agent/new/delay-agent /vz/private/$i/usr/local/etc/emulab");
    print "Copying $pairs-$i\n";
}
