#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#
use English;
use Getopt::Std;

#
# The point of this is to fire up the init code inside the jail,
# and then wait for a signal from outside the jail. When that happens
# kill off everything inside the jail and exit. So, like a mini version
# of /sbin/init, since killing the jail cleanly from outside the jail
# turns out to be rather difficult, and doing it from inside is very easy!
#
my $DEFCONSIX = "/bin/sh /etc/rc";

#
# Catch TERM.
# 
sub handler () {
    $SIG{TERM} = 'IGNORE';
    system("kill -TERM -1");
    sleep(1);
    system("kill -KILL -1");
    exit(1);
}
$SIG{TERM} = \&handler;

my $childpid = fork();
if (!$childpid) {
    if (@ARGV) {
	exec @ARGV;
    }
    else {
	exec $DEFCONSIX;
    }
    die("*** $0:\n".
	"    exec failed: '@ARGV'\n");
}

#
# If a command list was provided, we wait for whatever it was to
# finish. Otherwise sleep forever. 
#
if (@ARGV) {
    waitpid($childpid, 0);
    $SIG{TERM} = 'IGNORE';
    system("kill -TERM -1");
    sleep(1);
    system("kill -KILL -1");
}
else {
    while (1) {
	system("/bin/sleep 10000");
    }
}
exit(0);
