#!/usr/bin/perl
use SemiModern::Perl;
use TBConfig;
use Data::Dumper;
use Test::More tests => 1;


my $a = `./tbts -p DPRJ -g DGRP t/noautorun/tbts_cmdlineargs.t`;
ok(!$?, 'tbts cmdline args pass');
