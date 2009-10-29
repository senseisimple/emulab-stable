my @pairList = (1, 2, 5, 10, 15, 20, 25, 30, 35);

my $i = 0;
for ($i = 0; $i < scalar(@pairList); ++$i)
{
    my $pairs = $pairList[$i];
    system("ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "
	   . "vhost-0.virt$pairs.tbres.emulab.net "
	   . "'perl /proj/tbres/duerig/virt/copy-single-delay.pl $pairs'");
}
