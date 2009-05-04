#!/usr/bin/perl

$command = 'scp demo.swf duerig@boss.emulab.net:/usr/testbed/www/protogeni-demo-fed.swf';
print $command."\n";
system $command;

