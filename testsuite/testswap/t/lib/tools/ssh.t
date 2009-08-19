#!/usr/bin/perl
use SemiModern::Perl;
use TBConfig;
use Tools;
use Tools::TBSSH;
use Data::Dumper;
use Test::More tests => 7;

ok(0 == [Tools::TBSSH::cmdcheckoutput($TBConfig::OPS_SERVER, "hostname", sub { $_[0] =~ /ops.emulab.net/; } )]->[0], 'ssh ops hostname');
ok(1 == [Tools::TBSSH::cmdcheckoutput($TBConfig::OPS_SERVER, "false", sub { $_[2] } )]->[0], 'ssh ops false return code');
ok(0 == [Tools::TBSSH::cmdcheckoutput($TBConfig::OPS_SERVER, "true", sub { !$_[2]} )]->[0], 'ssh ops true return code');

#test instance method
my $ssh = Tools::TBSSH::instance($TBConfig::OPS_SERVER);
ok(0 == [Tools::TBSSH::cmdcheckoutput($TBConfig::OPS_SERVER, "hostname", sub { $_[0] =~ /ops.emulab.net/; } )]->[0], 'ssh ops hostname');

#test 
ok(0 == [$ssh->cmdsuccess("hostname", sub { $_[0] =~ /ops.emulab.net/; } )]->[0], 'cmdsuccess(..) test');
ok(0 == [$ssh->cmdsuccessdump("hostname", sub { $_[0] =~ /ops.emulab.net/; } )]->[0], 'cmdsuccessdump(..) test');

ok(0 == [$ssh->cmdsuccess_stdin("python -", 'print "Hello"' )]->[0], 'python stdin Hello test');
