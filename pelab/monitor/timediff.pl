#! /usr/bin/perl -w
use Getopt::Std;

getopts('d');
if ($#ARGV != 0) {
 print "usage: timediff.pl <remote-host>\n";
 exit;
}
$host = $ARGV[0];
$ntptrace = "ntptrace";
$cmd = "$ntptrace";
open(PH, $cmd . "|") || die "failed to start command $cmd: $!";
while (<PH>) {
  $stratum1 = $1 if (/\sstratum\s(\d+),/);
  $offset1 = $1 if (/\soffset\s([-\d.]+),/);
  print if ($opt_d);
}
close(PH) || die "$cmd failed";
if ($stratum1 != 1) {
  printf("Warning: the local host only reaches stratum %d \n", $stratum1);
}
print "=======================================================================================\n" if ($opt_d);
$cmd = "$ntptrace $host";
open(PH, $cmd . "|") || die "failed to start command $cmd: $!";
while (<PH>) {
  $stratum2 = $1 if (/\sstratum\s(\d+),/);
  $offset2 = $1 if (/\soffset\s([-\d.]+),/);
  print if ($opt_d);
}
close(PH) || die "$cmd failed";
if ($stratum2 != 1) {
  printf("Warning: the remote host %s only reaches stratum %d \n", $host, $stratum2);
}
print "=======================================================================================\n" if ($opt_d);
printf("The time difference between %s and the local host is %f\n", $host, $offset1-$offset2);

