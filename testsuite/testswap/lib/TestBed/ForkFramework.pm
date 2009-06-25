my $FFDEBUG = 0;

package TestBed::ForkFramework::Channel;
use SemiModern::Perl;
use Mouse;
use Data::Dumper;
use Carp;
use IO::Pipe;
use Storable qw(store_fd fd_retrieve);

has 'rd' => ( isa => 'Any', is => 'rw');
has 'wr' => ( isa => 'Any', is => 'rw');
has 'pipes' => ( isa => 'ArrayRef', is => 'rw');

sub build           { TestBed::ForkFramework::Channel->new( 'pipes' => [ map IO::Pipe->new, 1 .. 2 ]); }
sub parentAfterFork { my $s = shift; $s->buildChannel(@{ $s->pipes}); }
sub childAfterFork  { my $s = shift; $s->buildChannel(reverse(@{ $s->pipes})); }
sub buildChannel    { my $s = shift; $s->rd(shift->reader); $s->wr(shift->writer); }
sub receive         { receivefd(shift->rd); }
sub send            { sendfd(shift->wr, shift); }
sub receivefd       { my $fd = shift; my $r = fd_retrieve $fd;  return $r->[0]; }
sub sendfd          { my $fd = shift; store_fd [shift], $fd;    $fd->flush; }
sub sendEnd         { my $s = shift; $s->send(undef); $s->closeWr }
sub selectInit      { my $s = shift; return [ $s->rd, $s->wr, 0, $s ]; }
sub closeRd         { my $s = shift; my $fh = $s->rd; close($fh) if defined $fh; $s->rd(undef); }
sub closeWr         { my $s = shift; my $fh = $s->wr; close($fh) if defined $fh; $s->wr(undef); }
sub close           { my $s = shift; $s->closeRd; $s->closeWr; }

package TestBed::ForkFramework::Redir;
use SemiModern::Perl;
use Mouse;
use Carp;
use IO::Pipe;

has 'pipes' => ( isa => 'ArrayRef', is => 'rw', default => sub { [ map IO::Pipe->new, 1 .. 3 ] } );

sub parentAfterFork { my $ps = shift->pipes; $ps->[0]->writer;  $ps->[1]->reader;  $ps->[2]->reader; return wantarray ? @{$ps} : $ps; }
sub childAfterFork  {
  my $s = shift;
  my ($in, $out, $err) = @{$s->pipes};
  $in->reader;  $out->writer;  $err->writer;
  close STDIN; close STDOUT; close STDERR;
  open(STDIN,  "<&", $in ->fileno);
  open(STDOUT, ">&", $out->fileno);
  open(STDERR, ">&", $err->fileno);
  $s->close
}
sub close           { my $hs = shift->pipes; map { close $_; } @$hs; }

package TestBed::ForkFramework::Results;
use SemiModern::Perl;
use Mouse;

has 'successes' => ( isa => 'ArrayRef', is => 'rw', default => sub { [ ] } );
has 'errors' => ( isa => 'ArrayRef', is => 'rw', default => sub { [ ] } );

sub push_success { push @{shift->successes}, shift; }
sub push_error { push @{shift->errors}, shift; }
sub has_errors { scalar @{shift->errors};}
sub handle_result { 
  my ($self, $result) = @_;
  if   ( $result->is_error ) { $self->push_error($result); } 
  else { $self->push_success($result); } 
}

package TestBed::ForkFramework::ItemResult;
use SemiModern::Perl;
use Mouse;

has 'result' => ( is => 'rw');
has 'error'  => ( is => 'rw');
has 'itemid' => ( is => 'rw');

sub is_error { shift->error; }

use SemiModern::Perl;
use Mouse;

package TestBed::ForkFramework;
sub forkit {
  my ($parent_worker, $worker) = @_;

  if ( my $pid = fork ) {
    #Parent
    return $parent_worker->($pid);
  }
  else {
    #Child
    use POSIX '_exit';
    eval q{END { _exit 0 }};
  
    $worker->();

    CORE::exit;
  }
}

sub fork_child_redir {
  my ($worker) = @_;
  fork_redir( sub { return @_; }, $worker);
}

sub fork_redir {
  my ($parent_worker, $worker) = @_;
  my $redir = TestBed::ForkFramework::Redir->new;
  forkit( sub {
    #Parent
    my ($pid) = @_;
    my $handles = $redir->parentAfterFork;
    return $parent_worker->(@$handles, $pid);
  },
  sub {
    $redir->childAfterFork;
    $worker->();
  }
  );
}

package TestBed::ForkFramework::Scheduler;
use SemiModern::Perl;
use Mouse;
use IO::Select;
use Carp;
use Data::Dumper;

has 'workers'    => ( is => 'rw', default => sub { [] });
has 'results'    => ( is => 'rw', default => sub { TestBed::ForkFramework::Results->new; });
has 'selector'   => ( is => 'rw', default => sub { IO::Select->new; });
has 'items'      => ( is => 'rw', isa => 'ArrayRef', required => 1 );
has 'proc'       => ( is => 'rw', isa => 'CodeRef' , required => 1 );


sub _gen_iterator {
  my $items = shift;
  my @ar = @$items;
  my $pos = 0;
  return sub {
    return if $pos >= @ar;
    my @r = ( $pos, $ar[$pos] );
    $pos++;
    return @r;
  }
}

sub wait_for_all_children_to_exit {
  my ($self) = @_;
  waitpid( $_, 0 ) for @{ $self->workers };
}

sub workloop {
  my ($self) = @_;
  LOOP: {
    while( defined ( my $jobid = $self->spawnWorker ) ) {
      say "spawnWorker $jobid" if $FFDEBUG;
      $self->fffork($jobid);
    }
    say "CALL SELECT" if $FFDEBUG;
    if ($self->selectloop) {
      redo LOOP;
    }
  }
  $self->wait_for_all_children_to_exit;

  return $self->results;
}

use constant SELECT_HAS_HANDLES => 1;
use constant SELECT_NO_HANDLES  => 0;

sub selectloop {
  my ($self) = @_;
  my $selector = $self->selector;
  if ($selector->count) {
    eval {
      for my $r ($selector->can_read) {
        my ($rh, $wh, $eof, $ch) = @$r;
        if (defined (my $result = $ch->receive)) {
          $self->handleResult($result);
          
          unless ( $eof ) {
            if( my $jobid = $self->nextJob ) { 
              say "newjob $jobid" if $FFDEBUG;
              $ch->send($jobid); }
            else {
              say "no work killing $rh" if $FFDEBUG;
              $ch->sendEnd;
              @{$r}[1,2] = (undef, 1);            
            }
          }
        }
        else {
          say "received null ack from $rh" if $FFDEBUG;
          $selector->remove($r);
          $ch->close;
        }
      }
    };
    if ( my $error = $@ ) {
      say "SELECT HAS ERRORS" if $FFDEBUG;
      $_->[3]->sendEnd for $selector->handles;
      $self->wait_for_all_children_to_exit;
      die $error;
    }
    say "SELECT_HAS_HANDLES" if $FFDEBUG;
    return SELECT_HAS_HANDLES;
  }
  say "SELECT_NO_HANDLES" if $FFDEBUG;
  return SELECT_NO_HANDLES;
}

sub fffork {
  my ($self, $workid) = @_;
  my $ch = TestBed::ForkFramework::Channel::build;

  if ( my $pid = fork ) {
    #Parent
    $ch->parentAfterFork;
    push @{ $self->workers }, $pid;
    $self->selector->add($ch->selectInit);
    $ch->send($workid);
  }
  else {
    #Child
    $ch->childAfterFork;
    
    use POSIX '_exit';
    eval q{END { _exit 0 }};

    while ( defined( my $itemid = $ch->receive )) {
      my $result = eval { $self->doItem($itemid); };
      my $error  = $@;
      $ch->send(TestBed::ForkFramework::ItemResult->new(itemid => $itemid, result => $result, error => $error));
    }
    $ch->sendEnd;
    $ch->close;

    CORE::exit;
  }
}

sub doItem { my ($s, $itemid) = @_; $s->proc->($s->items->[$itemid]); }
sub handleResult { recordResult(@_); }
sub recordResult { shift->results->handle_result(shift); }

package TestBed::ForkFramework::ForEach;
use SemiModern::Perl;
use Mouse;

has 'iter'       => ( isa => 'CodeRef'  , is => 'rw', required => 1);

extends 'TestBed::ForkFramework::Scheduler';

sub spawnWorker { shift->nextJob; }
sub nextJob { 
  my @res = shift->iter->();
  $res[0];
}

sub work {
  my ($proc, $items) = @_;
  my $s = TestBed::ForkFramework::ForEach->new(
    'items' => $items,
    'proc' => $proc,
    'iter' => TestBed::ForkFramework::Scheduler::_gen_iterator($items),
  );
  $s->workloop;
}

package TestBed::ForkFramework::MaxWorkersScheduler;
use SemiModern::Perl;
use Mouse;

has 'maxworkers'  => ( isa => 'Int' , is => 'rw', default => 4);
has 'pos'         => ( isa => 'Int' , is => 'rw', default => 0);
has 'currworkers' => ( isa => 'Int' , is => 'rw', default => 0);

extends 'TestBed::ForkFramework::Scheduler';
      
sub work {
  my ($max_workers, $proc, $items) = @_;
  my $s = TestBed::ForkFramework::MaxWorkersScheduler->new(
    'maxworkers' => $max_workers, 
    'items' => $items,
    'proc' => $proc,
  );
  $s->workloop;
}

sub spawnWorker { 
  my $s = shift; 
  return if ($s->currworkers >= $s->maxworkers);
  $s->{'currworkers'}++;
  $s->nextJob; 
}

sub nextJob { 
  my $s = shift; 
  my $pos = $s->pos;
  return if ($pos >= scalar @{ $s->items });
  $s->{'pos'}++; 
  $pos;
}

package TestBed::ForkFramework::RateScheduler;
use SemiModern::Perl;
use Data::Dumper;
use Tools;
use Mouse;

extends 'TestBed::ForkFramework::Scheduler';

has 'maxnodes'  => ( isa => 'Int'      , is => 'rw', default => 20);
has 'currnodes' => ( isa => 'Int'      , is => 'rw', default => 0);
has 'schedule'  => ( isa => 'ArrayRef' , is => 'rw', required => 1);
has 'weight'    => ( isa => 'ArrayRef' , is => 'rw', required => 1);
has 'retryItems'=> ( isa => 'ArrayRef' , is => 'rw', default => sub { [] } );
has 'inRetry'   => ( isa => 'Int'      , is => 'rw', default => 0);


sub incr_currnodes {
  my ($s, $quantity) = @_;
  $s->{'currnodes'} += $quantity;
}

sub return_node_resources {
  my ($s, $itemid) = @_;
  $s->{'currnodes'} -= $s->weight->[$itemid];
}

sub work {
  my ($max_nodes, $proc, $schedule, $items) = @_;
  my $s = TestBed::ForkFramework::RateScheduler->new(
    'maxnodes' => $max_nodes, 
    'items' => $items,
    'proc' => $proc,
    'schedule' => $schedule,
    'weight' => [ map { $_->[0] } (sort { $a->[1] <=> $b->[1] } @$schedule) ],
  ); 
  say toperl("SCHEDULE", $s->schedule) if $FFDEBUG;
  say toperl("WEIGHTS", $s->weight) if $FFDEBUG;
  $s->workloop;
  say("RETRYING") if $FFDEBUG;
  $s->inRetry(1);
  $s->schedule( [ map { [$s->weight->[$_], $_] } @{$s->retryItems} ] );
  $s->retryItems([]);
  say toperl("SCHEDULE", $s->schedule) if $FFDEBUG;
  say toperl("WEIGHTS", $s->weight) if $FFDEBUG;
  $s->workloop;
}

sub find_largest_item {
  my ($s, $max_size) = @_;
  my $found = undef;

  #find largest item that is small enough
  for (@{ $s->schedule }) {
    my $itemsize = $_->[0];

    last if $itemsize > $max_size;
    next if ($found and $found->[0] >= $itemsize);
    $found = $_ if $itemsize <= $max_size;
  }

  #remove found from schedule
  if (defined $found) {
    $s->schedule( [ grep { !($_->[1] == $found->[1]) } @{ $s->schedule} ]);
  }

  return $found;
}

sub spawnWorker { shift->nextJob; }
sub nextJob { 
  my $s = shift;
  my $max_size = $s->maxnodes - $s->currnodes; 
  my $tuple = $s->find_largest_item($max_size);

  if ($tuple) {
    my ($e_node_size, $eindex) = @$tuple;
    say(sprintf("found %s size %s max_size $max_size currnodes %s maxnodes %s newcurrnodes %s", $eindex, $e_node_size, $s->currnodes, $s->maxnodes, $s->currnodes +$e_node_size)) if $FFDEBUG;
    $s->{'currnodes'} += $e_node_size;
    return $eindex;
  }
  else { return; }
}

use TestBed::ParallelRunner::ErrorConstants;

sub return_and_report {
  my ($s, $result) = @_;
  $s->recordResult($result);
  $s->return_node_resources($result->itemid);
}

sub handleResult { 
  my ($s, $result) = @_;
  my $executor = $s->items->[$result->itemid];
  if ($executor->can('handleResult')) {
    my $rc = $executor->handleResult($s, $result);
    if ($rc == RETURN_AND_REPORT) { $s->return_and_report($result) }
  }
  else {
    $s->return_and_report($result);
  }
}

sub retry {
  my ($s, $result) = @_;
  my $itemid = $result->itemid;
  if (!$s->inRetry) {
    push @{ $s->retryItems }, $itemid;
    $s->return_node_resources($itemid);
    say "RETRYING item# $itemid";
    return 1;
  }
  else { 
    say "DONE RETRYING";
    $s->return_and_report($result); 
  }
}

1;
