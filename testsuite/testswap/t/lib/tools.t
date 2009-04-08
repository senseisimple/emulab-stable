#!/usr/bin/perl
use SemiModern::Perl;
use TBConfig;
use Tools;
use Tools::TBSSH;
use Data::Dumper;
use Test::More tests => 4;

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
ok(concretize($a, OS=>'RedHatAnchient') eq $b);

ok(0 == [Tools::TBSSH::cmdcheckoutput($TBConfig::OPS_SERVER, "hostname", sub { $_[0] =~ /ops.emulab.net/; } )]->[0]);
ok(1 == [Tools::TBSSH::cmdcheckoutput($TBConfig::OPS_SERVER, "false", sub { $_[2] } )]->[0]);
ok(0 == [Tools::TBSSH::cmdcheckoutput($TBConfig::OPS_SERVER, "true", sub { !$_[2]} )]->[0]);
