package TestBed::ForkFramework::Channel;
use SemiModern::Perl;
use Mouse;
use Data::Dumper;
use Storable qw(store_fd fd_retrieve);

has 'rd' => ( isa => 'Any', is => 'rw');
has 'wr' => ( isa => 'Any', is => 'rw');

sub build      { TestBed::ForkFramework::Channel->new( 'rd' => shift, 'wr' => shift ); }
sub receive    { receivefd(shift->rd); }
sub send       { sendfd(shift->wr, shift); }
sub receivefd  { my $fd = shift; my $r = fd_retrieve $fd;  return $r->[0]; }
sub sendfd     { my $fd = shift; store_fd [shift], $fd;    $fd->flush; }
sub sendEnd    { my $self = shift; $self->send(undef); $self->closeWr }
sub sendError  { shift->send( [ 'E', @_ ] ) }
sub sendResult { shift->send( [ 'R', @_ ] ) }
sub selectInit { my $self = shift; return [ $self->rd, $self->wr, 0, $self ]; }
sub sendWorkStatus   { 
  my ($self, $error, $jobid, $result) = @_;
  if ($error) { $self->sendError($error, $jobid); }
  else        { $self->sendResult($result, $jobid); } 
}
sub closeRd    { my $self = shift; my $fh = $self->rd; close($fh) if defined $fh; $self->rd(undef); }
sub closeWr    { my $self = shift; my $fh = $self->wr; close($fh) if defined $fh; $self->wr(undef); }
sub close      { my $self = shift; $self->closeRd; $self->closeWr; }

package TestBed::ForkFramework::BiPipe;
use SemiModern::Perl;
use Mouse;
use Carp;
use IO::Pipe;

has 'pipes' => ( isa => 'ArrayRef', is => 'rw');

sub build           { TestBed::ForkFramework::BiPipe->new( 'pipes' => [ map IO::Pipe->new, 1 .. 2 ]); }
sub parentAfterFork { buildChannel(@{shift->pipes}); }
sub childAfterFork  { buildChannel(reverse(@{shift->pipes})); }
sub buildChannel    { my ($rd, $wr) = @_; $rd->reader; $wr->writer; TestBed::ForkFramework::Channel::build($rd, $wr); }

package TestBed::ForkFramework::Redir;
use SemiModern::Perl;
use Mouse;
use Carp;
use IO::Pipe;

has 'pipes' => ( isa => 'ArrayRef', is => 'rw');

sub build { TestBed::ForkFramework::Redir->new( 'pipes' => [ map IO::Pipe->new, 1 .. 3 ]); }
sub parentAfterFork { my $ps = shift->pipes; $ps->[0]->writer;  $ps->[1]->reader;  $ps->[2]->reader; return wantarray ? @{$ps} : $ps; }
sub childAfterFork  { 
  my $s = shift;
  my ($in, $out, $err) = @{$s->pipes};
  $in->reader;  $out->writer;  $err->writer; 
  close STDIN;
  close STDOUT; 
  close STDERR;
  open(STDIN,  "<&", $in ->fileno);
  open(STDOUT, ">&", $out->fileno); 
  open(STDERR, ">&", $err->fileno);
  $s->close
}
sub handles         { my $hs = shift->pipes; return wantarray ? @{$hs} : $hs; }
sub close           { my $hs = shift->pipes; map { close $_; } @$hs; }
sub in              { shift->pipes->[0]; }
sub out             { shift->pipes->[1]; }
sub err             { shift->pipes->[2]; }

package TestBed::ForkFramework::Scheduler;
use SemiModern::Perl;
use Mouse;
use IO::Select;
use Carp;
use Data::Dumper;

has 'workers'    => ( is  => 'rw');
has 'results'    => ( is  => 'rw');
has 'errors'     => ( is  => 'rw');
has 'selector'   => ( is  => 'rw');
has 'items'      => ( isa => 'ArrayRef' , is => 'rw');
has 'proc'       => ( isa => 'CodeRef'  , is => 'rw');

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

sub workloop {
  my ($self) = @_;
  LOOP: {
    while( defined ( my $jobid = $self->spawnWorker ) ) {
      $self->fffork($jobid);
    }
    if ($self->selectloop) {
      redo LOOP;
    }
  }
  waitpid( $_, 0 ) for @{ $self->workers };

  my @results = (scalar @{$self->errors}, $self->results, $self->errors);
  return wantarray ? @results : \@results;
}

use constant SELECT_HAS_HANDLES  => 1;
use constant SELECT_NO_HANDLES  => 0;

sub selectloop {
  my ($self) = @_;
  my $selector = $self->selector;
  if ($selector->count) {
    eval {
      for my $r ($selector->can_read) {
        my ($rh, $wh, $eof, $ch) = @$r;
        if (defined (my $result = $ch->receive)) {
          my $type = shift @$result;
          if    ( $type eq 'R' ) { push @{ $self->results }, $result }
          elsif ( $type eq 'E' ) { push @{ $self->errors  }, $result }
          else { die "Bad result type: $type"; }
          $self->jobDone($result->[1]);
          
          unless ( $eof ) {
            if( my $jobid = $self->nextJob ) { $ch->send($jobid); }
            else {
              $ch->sendEnd;
              @{$r}[1,2] = (undef, 1);            
            }
          }
        }
        else {
          $selector->remove($r);
          $ch->close;
        }
      }
    };
    if ( my $error = $@ ) {
      $_->[3]->sendEnd for $selector->handles;
      waitpid( $_, 0 ) for @{ $self->workers };
      die $error;
    }
    return SELECT_HAS_HANDLES;
  }
  return SELECT_NO_HANDLES;
}

sub fffork {
  my ($self, $workid) = @_;
  my $bipipe = TestBed::ForkFramework::BiPipe::build;

  if ( my $pid = fork ) {
    #Parent
    my $ch = $bipipe->parentAfterFork;
    push @{ $self->workers }, $pid;
    $self->selector->add($ch->selectInit);
    $ch->send($workid);
  }
  else {
    #Child
    my $ch = $bipipe->childAfterFork;
    
    use POSIX '_exit';
    eval q{END { _exit 0 }};

    while ( defined( my $itemid = $ch->receive )) {
      my $result = eval { $self->doItem($itemid); };
      my $error  = $@;
      $ch->sendWorkStatus($error, $itemid, $result);
    }
    $ch->sendEnd;
    $ch->close;

    CORE::exit;
  }
}

sub redir_std_fork {
  my ($self, $pworker, $worker) = @_;
  my $redir = TestBed::ForkFramework::Redir::build;

  if ( my $pid = fork ) {
    #Parent
    my $handles = $redir->parentAfterFork;

    $pworker->(@$handles, $pid);

    waitpid($pid, 0);
  }
  else {
    #Child
    $redir->childAfterFork;
    
    use POSIX '_exit';
    eval q{END { _exit 0 }};
  
    $worker->();

    CORE::exit;
  }
}

sub doItem { my ($s, $itemid) = @_; $s->proc->($s->items->[$itemid]); }
sub jobDone { }

package TestBed::ForkFramework::ForEach;
use SemiModern::Perl;
use Mouse;

has 'iter'       => ( isa => 'CodeRef'  , is => 'rw');

extends 'TestBed::ForkFramework::Scheduler';

sub spawnWorker { shift->nextJob; }
sub nextJob { 
  my @res = shift->iter->();
  $res[0];
}

sub work {
  my ($proc, $items) = @_;
  my $s = TestBed::ForkFramework::ForEach->new(
    'workers' => [],
    'results' => [],
    'items' => $items,
    'errors' => [],
    'proc' => $proc,
    'iter' => TestBed::ForkFramework::Scheduler::_gen_iterator($items),
    'selector' => IO::Select->new);
  $s->workloop;
}

package TestBed::ForkFramework::MaxWorkersScheduler;
use SemiModern::Perl;
use Mouse;

has 'maxworkers'  => ( isa => 'Int'      , is => 'rw');
has 'pos'         => ( isa => 'Int'      , is => 'rw');
has 'currworkers' => ( isa => 'Int'      , is => 'rw');

extends 'TestBed::ForkFramework::Scheduler';
      
sub work {
  my ($max_workers, $proc, $items) = @_;
  my $s = TestBed::ForkFramework::MaxWorkersScheduler->new(
    'maxworkers' => $max_workers, 
    'currworkers' => 0,
    'workers' => [],
    'results' => [],
    'items' => $items,
    'errors' => [],
    'proc' => $proc,
    'pos' => 0,
    'selector' => IO::Select->new);
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

has 'maxnodes'  => ( isa => 'Int'      , is => 'rw');
has 'currnodes' => ( isa => 'Int'      , is => 'rw');
has 'schedule' => ( isa => 'ArrayRef', is => 'rw');
has 'weight' => ( isa => 'ArrayRef', is => 'rw');

sub work {
  my ($max_nodes, $proc, $weight, $items) = @_;
  my $s = TestBed::ForkFramework::RateScheduler->new(
    'maxnodes' => $max_nodes, 
    'currnodes' => 0,
    'workers' => [],
    'results' => [],
    'items' => $items,
    'errors' => [],
    'proc' => $proc,
    'schedule' => $weight,
    'weight' => [ map { $_->[0] } (sort { $a->[1] <=> $b->[1] } @$weight) ],
    'selector' => IO::Select->new);
  #sayperl($s->schedule);
  #sayperl($s->weight);
  $s->workloop;
}

sub find_largest_item {
  my ($s, $max_size) = @_;
  my $found = undef;

  #find largest item that is small enough
  for (@{ $s->schedule }) {
    $found = $_ if $_->[0] <= $max_size;
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
    #say sprintf("found %s size %s max_size $max_size currnodes %s maxnodes %s", $eindex, $e_node_size, $s->currnodes, $s->maxnodes);
    $s->{'currnodes'} += $e_node_size;
    return $eindex;
  }
  else {
    return;
  }
}
sub jobDone { my ($s, $itemid) = @_;  $s->{'currnodes'} -= $s->weight->[$itemid];  }
1;
