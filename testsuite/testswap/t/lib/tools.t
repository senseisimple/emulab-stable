#!/usr/bin/perl
#
# EMULAB-COPYRIGHT
# Copyright (c) 2009 University of Utah and the Flux Group.
# All rights reserved.
#
use SemiModern::Perl;
use TBConfig;
use Tools;
use Tools::TBSSH;
use Data::Dumper;
use Test::More tests => 1;

my $a = <<'END';
Dooh
@OS@
Dooh
END
my $b = <<'END';
Dooh
RedHatAnchient
Dooh
END
ok(Tools::concretize($a, OS=>'RedHatAnchient') eq $b, 'concretize templating utility');
