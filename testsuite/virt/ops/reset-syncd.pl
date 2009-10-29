#!/usr/bin/perl

$pid = `cat /var/run/syncd.pid`;
$command = "sudo kill -s HUP $pid";
print "$command\n";
system($command);
