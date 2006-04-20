#! /usr/bin/perl -w
use Getopt::Std;

our ($opt_d, $opt_n);
getopts('dn:');
if ($#ARGV != 0) {
 print "usage: timediff.pl [-d] [-n <sample number>] <remote-host>\n";
 exit;
}
$host = $ARGV[0];
$ntptrace = "/proj/tbres/bin/ntptrace";
$cmd = "$ntptrace -nv $host";
$sampnum = 20;
if ($opt_n) {
  if ($opt_n !~ /^\d+$/) {
    print "Error: -n option must be a positive integer!\n";
    exit -1;
  }
  $sampnum = $opt_n;
}
$mindelay = 100000; 
for ($i=0; $i<$sampnum; $i++) {
  open(PH, $cmd . "|") || die "failed to start command $cmd: $!";
  $lineno = 1;
  while (<PH>) {
    print if ($opt_d);
    if ($lineno == 3) {
      $delay = $1 if (/\sdelay\s(-?\d+\.?\d*),/);
      $offset = $1 if (/\soffset\s(-?\d+\.?\d*)$/);
      last if (!$opt_d);
    }
    $lineno++;
  }
  close(PH) || die "$cmd failed";
  if (!defined($delay) || !defined($offset)) {
    print "Error: delay or offset is not found in the ntptrace output!\n";
    exit -1;
  }
  if ($opt_d) {
    printf("delay: %f, offset: %f \n", $delay, $offset);
    printf("======================================================================\n"); 
  }
  if ($delay < $mindelay){
    $mindelay  = $delay;
    $minoffset = $offset;
  } 
}
printf("The min delay is %f, and the corresponding offset is %f\n", $mindelay, $minoffset);

