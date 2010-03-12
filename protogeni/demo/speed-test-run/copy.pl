#!/usr/bin/perl

$command = 'scp speed-test-run.swf duerig@boss.emulab.net:/usr/testbed/www/speed-test-run.swf';
print $command."\n";
system $command;

