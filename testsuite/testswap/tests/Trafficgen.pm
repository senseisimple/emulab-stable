#
# EMULAB-COPYRIGHT
# Copyright (c) 2009 University of Utah and the Flux Group.
# All rights reserved.
#
package Event;
use SemiModern::Perl;
use Mouse;

has 'start' => (is => 'rw');
has 'end'   => (is => 'rw');
has 'qty'   => (is => 'rw');

sub update {
  my ($s, $ts, $qty) = @_;
  $s->end($ts);
  $s->qty($qty + $s->qty);
}

package TrafficGenLister;
use SemiModern::Perl;
use Mouse;
use IO::Socket::INET;
use IO::Handle;
use Test::More;

has 'state' => (is=>'rw', 'default' => sub { [] } );
has 'events' => (is=>'rw', 'default' => sub { [ [], [] ] } );
has 'basetime' => (is=>'rw');
has 'socket' => (is=>'rw');
has 'traffic' => (is=>'rw', 'default' => sub { [] } );

sub init {
  my ($s, $eid, $pid) = @_;
  my $socket = IO::Socket::INET->new(Proto => 'tcp', PeerAddr => "node0.$eid.$pid.emulab.net:4443" ) or die "Socket Error: $@\n";
  $socket->getline;
  $socket->getline;
  $s->socket($socket);
}

sub normalizeTimes {
  my ($s, $ts) = @_;
  return $ts - $s->basetime;
}

sub recordEvent {
  my ($s, $id, $ts, $qty) = @_;
  if    ( !$qty and !$s->state->[$id] ) { }                                                                               # noop
  elsif (  $qty and !$s->state->[$id] ) { $s->state->[$id] = Event->new(start => $s->normalizeTimes($ts), qty => $qty); } # event start
  elsif (  $qty and  $s->state->[$id] ) { $s->state->[$id]->update($s->normalizeTimes($ts), $qty); }                      # event update
  elsif ( !$qty and  $s->state->[$id] ) { push @{ $s->events->[$id] }, $s->state->[$id]; $s->state->[$id] = undef; }                          # event end
}

sub parseline {
  local $^W = 1;
  use Carp qw(confess);
  my $s = shift;
  my $l = $s->socket->getline;
  print $l;
  my ($ts, $tcp, $udp) = ($l =~ /(\d+)\.\d+ \(icmp:\d+,\d+ tcp:(\d+),\d+ udp:(\d+),\d+ other:\d+,\d+\)/);

  if (! defined $s->basetime) {
    return $s->basetime($ts);
  }

  #event id 0 is tcp, 1 is udp
  $s->recordEvent(0, $ts, $tcp);
  $s->recordEvent(1, $ts, $udp);
}


sub print { shift->socket->print(shift); }
sub close { 
  my $s = shift;
  $s->socket->close; 
  $s->recordEvent(0, 0, 0);
  $s->recordEvent(1, 0, 0);
}

sub check {
  my ($s, $id, $start, $expected_duration, $bytes, $desc) = @_;
  #sayd($s->events->[$id]);
  my ($event) = grep { 
    #say $_->start, " ", $start, " ", ($_->start - $start); 
    abs($_->start - $start) <= 1; } @{$s->events->[$id]};
  die "*** Could not find traffic corresponding to \"$desc\"\n" unless (defined $event);
  my $actual_duration = ($event->end - $event->start);
  if (abs(($actual_duration - $expected_duration)) > 1) {
    die "*** Traffic from \"$desc\" lasted for $actual_duration seconds when it was only suppose to last for $expected_duration seconds\n";
  }
  my $actual_bytes = $event->qty;
  my $tol = 0.20;
  if (abs($actual_bytes - $bytes) > $bytes*$tol) {
    warn "*** Traffic from \"$desc\" generated $actual_bytes bytes of data but was expecting approximately $bytes bytes with a tolerance of " .  sprintf("%.0f%%\n", $tol * 100);
  }
}

sub check_ok {
  my ($s, $id, $start, $expected_duration, $bytes, $desc) = @_;
  check($s, $id, $start, $expected_duration, $bytes, $desc);
  ok(1, $desc);
}

package ImageTests;
use SemiModern::Perl;
use TestBed::TestSuite;
use Test::More;
use TestBed::ForkFramework;
my $TrafficGen = <<'END';
set ns [new Simulator]
source tb_compat.tcl

set node0 [$ns node]
set node1 [$ns node]

tb-set-node-os $node0 @OS@
tb-set-node-os $node1 @OS@

set link1 [$ns duplex-link $node0 $node1 100Mb 0ms DropTail]

set udp0 [new Agent/UDP]
$ns attach-agent $node0 $udp0

set cbr_udp [new Application/Traffic/CBR]
$cbr_udp set packetSize_ 500
$cbr_udp set interval_ 0.05
$cbr_udp attach-agent $udp0

set null0 [new Agent/Null]
$ns attach-agent $node1 $null0

$ns connect $udp0 $null0

set tcp0 [new Agent/TCP]
$ns attach-agent $node1 $tcp0

set cbr_tcp [new Application/Traffic/CBR]
$cbr_tcp set packetSize_ 500
$cbr_tcp set interval_ 0.01
$cbr_tcp attach-agent $tcp0

set null1 [new Agent/Null]
$ns attach-agent $node0 $null1

$ns connect $tcp0 $null1

set tl [$ns event-timeline]

$tl at 10.0 "$cbr_udp start"
$tl at 12.0 "$cbr_udp stop"
$tl at 15.0 "$cbr_tcp start"
$tl at 17.0 "$cbr_tcp stop"

$tl at 20.0 "$cbr_udp start"
$tl at 20.0 "$cbr_tcp start"
$tl at 22.0 "$cbr_udp stop"
$tl at 22.0 "$cbr_tcp stop"

$link1 trace
$link1 trace_endnode 1

$ns rtproto Static
$ns run
END

sub traffic_gen_test {
  my $e = shift;
  my $eid = $e->eid;
  my $pid = $e->pid;
  my @n = map { my $s = TrafficGenLister->new; $s->init($eid, $pid); $s; } (1..2);

  my $ppid = TestBed::ForkFramework::spawn( sub { $e->tevc("-w now tl start"); } );

  map { $_->print("1000\n"); } @n;

  while ($ppid->isalive) {
    for (@n) {
      $_->parseline;
    }
  };

  die "tevc existed with non-zero status: $?\n" if ($? >> 8 != 0);

  for (@n) { 
    $_->parseline; 
    $_->close;
    $_->check_ok(1, 10, 2, 20000, '$tl at 10.0 "$cbr_udp start"');
    $_->check_ok(1, 20, 2, 20000, '$tl at 20.0 "$cbr_udp start"');
    $_->check_ok(0, 15, 2, 10000, '$tl at 15.0 "$cbr_tcp start"');
    $_->check_ok(0, 20, 2, 10000, '$tl at 20.0 "$cbr_tcp start"');
  }
}

my $OS ="RHL90-STD";

{
  my $eid = 'trafficgen';
  my $ns = concretize($TrafficGen, OS => $OS);
  my $test_sub = sub { my $e = shift; traffic_gen_test($e);};
  rege(e($eid), $ns, $test_sub, 8, 'ImageTest-trafficgen test');
}

1;
