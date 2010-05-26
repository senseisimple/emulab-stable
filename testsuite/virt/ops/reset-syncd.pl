#!/usr/bin/perl
#
# EMULAB-COPYRIGHT
# Copyright (c) 2009 University of Utah and the Flux Group.
# All rights reserved.
#

$pid = `cat /var/run/syncd.pid`;
$command = "sudo kill -s HUP $pid";
print "$command\n";
system($command);
