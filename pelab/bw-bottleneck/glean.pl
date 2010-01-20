#!/usr/bin/perl

$dumpPattern = '^([0-9.]*) IP [0-9.]+ > ([0-9.]+): [A-Za-z. ]+([0-9]+)';

while ($current = <STDIN>)
{
    $current =~ /$dumpPattern/;
    print "$1 $2 $3\n";
}
