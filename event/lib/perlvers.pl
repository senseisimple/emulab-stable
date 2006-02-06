#!/usr/bin/perl -w
use English;

#
# A silly little script to figure out what version of perl is running
# so we can find headers for the swig generated goo.
#
foreach my $p (@INC) {
    if ($p =~ /perl5\/(\d+\.\d+\.\d+)\//) {
	print "$1";
	exit(0);
    }
}
exit(1);
