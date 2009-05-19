#!/usr/bin/perl
use SemiModern::Perl;
use TestBed::TestSuite;
use BasicNSs;
use Test::More;
use Data::Dumper;

my $linkupdowntest = sub {
  my ($e) = @_; 
  my $eid = $e->eid;
  ok($e->linktest, "$eid linktest"); 

  ok($e->link("link1")->down, "link down");
  sleep(2);

  my $n1ssh = $e->node("node1")->ssh;
  ok($n1ssh->cmdfailuredump("ping -c 5 10.1.2.3"));

  ok($e->link("link1")->up, "link up");
  sleep(2);
  ok($n1ssh->cmdsuccessdump("ping -c 5 10.1.2.3"));
};

rege('linkupdown', $BasicNSs::TwoNodeLanWithLink, $linkupdowntest, 5, 'link up and down with ping on link');

my $twonodelan5Mbtest = sub {
  my ($e) = @_; 
  my $eid = $e->eid;
  ok($e->linktest, "$eid linktest"); 
};

rege('2nodelan5Mb', $BasicNSs::TwoNodeLan5Mb, $twonodelan5Mbtest, 1, 'two node 5mb lan pingswapkill');
rege('singlenode', $BasicNSs::SingleNode, sub { ok(shift->pingswapkill); }, 1, 'single node pingswapkill');
rege('2nodelan', $BasicNSs::TwoNodeLan, sub { ok(shift->pingswapkill); }, 1, 'two node lan pingswapkill');

1;
