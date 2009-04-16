#!/usr/bin/perl
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
ok(concretize($a, OS=>'RedHatAnchient') eq $b, 'concretize templating utility');
