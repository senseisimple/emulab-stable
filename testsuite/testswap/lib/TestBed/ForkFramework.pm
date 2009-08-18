my $FFDEBUG = 0;

package TestBed::ForkFramework::Process;
use SemiModern::Perl;
use Mouse;
use POSIX ":sys_wait_h";
has 'pid' => (is => 'rw');

sub isalive { waitpid(shift->pid, WNOHANG) <= 0; }
sub wait    { waitpid(shift->pid, 0); }

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
sub selectInit      { my $s = shift; return [ $s->rd, $s->wr, 0, $s, shift ]; }
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
sub successful { ! shift->has_errors;}
sub sorted_results {
  return map { $_->result } (sort { $a->itemid <=> $b->itemid } @{shift->successes});
}
sub handleResult { 
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
has 'name'   => ( is => 'rw');

sub is_error { shift->error; }
sub error_type { ref(shift->error); }

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

sub spawn {
  my ($worker) = @_;
  forkit( sub { return TestBed::ForkFramework::Process->new( pid => $_[0]); } , $worker);
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

sub fork_child_redir {
  my ($worker) = @_;
  fork_redir( sub { return @_; }, $worker);
}

package TestBed::ForkFramework::Scheduler;
use SemiModern::Perl;
use Mouse;
use IO::Select;
use Carp;
use Data::Dumper;
use POSIX ":sys_wait_h";

has 'workers'       => ( is => 'rw', default => sub { [] });
has 'results'       => ( is => 'rw', default => sub { TestBed::ForkFramework::Results->new; });
has 'selector'      => ( is => 'rw', default => sub { IO::Select->new; });
has 'selecttimeout' => ( is => 'rw', default => 10 ); #seconds
has 'proc'          => ( is => 'rw', isa => 'CodeRef' , required => 1 );


sub wait_for_all_children_to_exit {
  my ($s) = @_;
  waitpid( $_, 0 ) for @{ $s->workers };
}

sub reap_zombies {
  my ($s) = @_;
  while ((my $child = waitpid(-1, WNOHANG)) > 0) {
    $s->workers( [ grep { !($_ == $child) } @{ $s->workers } ]);
  }
}

sub workloop {
  my ($self) = @_;
  LOOP: {
    while( defined ( my $jobid = $self->spawnWorker ) ) {
      say "spawnWorker $jobid" if $FFDEBUG;
      $self->fffork($jobid);
    }
    my $selectrc = $self->process_select; say "CALL SELECT" if $FFDEBUG;
    my $schedulerc = $self->schedule;

    $self->reap_zombies;

    if ($selectrc || $schedulerc) { redo LOOP; }
  }
  $self->wait_for_all_children_to_exit;

  return $self->results;
}

use constant SELECT_HAS_HANDLES => 1;
use constant SELECT_NO_HANDLES  => 0;

sub eval_report_error(&$) {
  my ($p, $m) = @_;
  eval { $p->(); };
  if ($@) {
    say $m;
    sayd($@);
  }
}

sub handle_select_error {
  my ($s, $r) = @_;
  my ($rh, $wh, $eof, $ch, $pid) = @$r;
  say "SELECT HAS EXCEPTION";
  sayd($r);

  eval_report_error { $ch->sendEnd; } "sendEnd";
  eval_report_error { $ch->close; } "chclose";
  eval_report_error { $s->selector->remove($r); } "selectorremove";
  eval_report_error { @{$r}[1,2] = (undef, 1); } "undefassign";
  eval_report_error { kill 9, $pid; } "kill $pid";

  say "DONE SELECT HAS EXCEPTION";
}

sub process_select {
  my ($self) = @_;
  my $selector = $self->selector;
  if ($selector->count) {
    my ($r, $rh, $wh, $eof, $ch, $pid);
    eval {
      for $r ($selector->has_exception(0)) {
        $self->handle_select_error($r);
      }
    };
    if ( my $error = $@ ) {
      say "SELECT HAS EXCEPTION ERRORS";
      sayd($error);
      sayd($r);
      say "DONE SELECT HAS EXCEPTION ERRORS";
    }
    eval {
      for $r ($selector->can_read($self->selecttimeout)) {
        ($rh, $wh, $eof, $ch, $pid) = @$r;
        if (defined (my $itemResult = $ch->receive)) {
          eval_report_error { $self->handleItemResult($itemResult); } 'ERROR $self->handleItemResult($itemResult);';
          
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
      say "SELECT HAS ERRORS";
      sayd($error);
      $self->handle_select_error($r);
      #$_->[3]->sendEnd for $selector->handles;
      #$self->wait_for_all_children_to_exit;
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
    $self->selector->add($ch->selectInit($pid));
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

sub doItem { die "HAVE TO IMPLEMENT doItem"; }
sub handleItemResult { recordItemResult(@_); }
sub recordItemResult { shift->results->handleResult(shift); }
sub schedule { 0; }

package TestBed::ForkFramework::ForEach;
use SemiModern::Perl;
use Mouse;

has 'maxworkers'  => ( is => 'rw', isa => 'Int'     , default => 4);
has 'currworkers' => ( is => 'rw', isa => 'Int'     , default => 0);
has 'iter'        => ( is => 'rw', isa => 'CodeRef' , required => 1);
has 'items'       => ( is => 'rw', isa => 'ArrayRef', required => 1 );

extends 'TestBed::ForkFramework::Scheduler';

sub spawnWorker { 
  my $s = shift; 
  return if ($s->currworkers >= $s->maxworkers);
  $s->{'currworkers'}++;
  $s->nextJob; 
}

sub nextJob { 
  my @res = shift->iter->();
  $res[0];
}

sub work {
  my ($proc, $items) = @_;
  return max_work(scalar @$items, $proc, $items);
}

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

sub max_work {
  my ($max_workers, $proc, $items) = @_;
  my $s = TestBed::ForkFramework::ForEach->new(
    'maxworkers' => $max_workers, 
    'items' => $items,
    'proc' => $proc,
    'iter' => _gen_iterator($items),
  );
  $s->workloop;
}

sub worksubs {
  my $items = [@_];
  return max_work(scalar @$items, sub { $_[0]->() } , $items);
}

sub doItem { my ($s, $itemid) = @_; $s->proc->($s->items->[$itemid]); }

package TestBed::ForkFramework::WeightedScheduler::Task;
use SemiModern::Perl;
use Mouse;

has 'id'      => (is => 'rw');
has 'item'    => (is => 'rw');
has 'runtime' => (is => 'rw', default => 0);
has 'weight'  => (is => 'rw', default => 0);

sub build {
  shift;
  return TestBed::ForkFramework::WeightedScheduler::Task->new(
    id => shift,
    item => shift, 
    weight => shift
  );
}

sub ready { return time >= shift->runtime; }

package TestBed::ForkFramework::WeightedScheduler;
use SemiModern::Perl;
use Data::Dumper;
use Tools;
use Mouse;

extends 'TestBed::ForkFramework::Scheduler';

has 'ids'  =>      ( isa => 'Int'      , is => 'rw', default => 0);
has 'maxnodes'  => ( isa => 'Int'      , is => 'rw', default => 20);
has 'currnodes' => ( isa => 'Int'      , is => 'rw', default => 0);
has 'runqueue'  => ( isa => 'ArrayRef' , is => 'rw', default => sub { [] } );
has 'tasks'     => ( isa => 'ArrayRef' , is => 'rw', default => sub { [] } );
has 'retryTasks'=> ( isa => 'ArrayRef' , is => 'rw', default => sub { [] } );
has 'waitTasks' => ( isa => 'ArrayRef' , is => 'rw', default => sub { [] } );
has 'inRetry'   => ( isa => 'Int'      , is => 'rw', default => 0);

sub nextID { 
  my ($s) = @_;
  my $id = $s->ids;
  $s->ids($id + 1);
  $id;
}

sub task { shift->tasks->[shift]; }

sub add_task {
  my ($s, $item, $weight) = @_;
  my $id = $s->nextID;
  my $task = TestBed::ForkFramework::WeightedScheduler::Task->build($id, $item, $weight);
  push @{$s->runqueue}, $task;
  $s->tasks->[$id] = $task;
}

sub incr_currnodes {
  my ($s, $quantity) = @_;
  $s->{'currnodes'} += $quantity;
}

sub return_node_resources {
  my ($s, $task) = @_;
  $s->{'currnodes'} -= $task->weight;
}

sub sort_runqueue {
  my ($s) = @_;
  $s->runqueue( [ sort { $a->weight <=> $b->weight } @{$s->runqueue} ] );
}

sub work {
  my ($maxnodes, $proc, $items_weights) = @_;
  my $s = TestBed::ForkFramework::WeightedScheduler->new(
    maxnodes => $maxnodes,
    proc => $proc,
  );
  $s->add_task($_->[0], $_->[1]) for (@$items_weights);
  $s->run;
}

sub run {
  my ($s) = @_;
  $s->sort_runqueue;
  $s->workloop;

  say("RETRYING") if $FFDEBUG;
  $s->inRetry(1);
  $s->runqueue( [ @{$s->retryTasks} ] );
  $s->retryTasks([]);

  $s->sort_runqueue;
  $s->workloop;
}

sub find_largest_item {
  my ($s, $max_weight) = @_;
  my $found = undef;

  #find largest task that is small enough
  for (@{ $s->runqueue }) {
    my $item_weight = $_->weight;

    last if $item_weight > $max_weight;
    next if ($found and $found->weight >= $item_weight);
    $found = $_ if $item_weight <= $max_weight;
  }

  #remove found from runqueue
  if (defined $found) {
    $s->runqueue( [ grep { !($_->id == $found->id) } @{ $s->runqueue} ]);
  }

  return $found;
}

sub spawnWorker { shift->nextJob; }
sub nextJob { 
  my $s = shift;
  my $max_size = $s->maxnodes - $s->currnodes; 
  my $task = $s->find_largest_item($max_size);

  if ($task) {
    say(sprintf("found %s size %s max_size $max_size currnodes %s maxnodes %s newcurrnodes %s", $task->id, $task->weight, $s->currnodes, $s->maxnodes, $s->currnodes + $task->weight)) if $FFDEBUG;
    $s->{'currnodes'} += $task->weight;
    return $task->id;
  }
  else { return; }
}
sub doItem { my ($s, $taskid) = @_; $s->proc->($s->tasks->[$taskid]->item); }

use TestBed::ParallelRunner::ErrorConstants;

sub return_and_report {
  my ($s, $result) = @_;
  $s->recordItemResult($result);
  $s->return_node_resources($s->task($result->itemid));
}

sub handleItemResult { 
  my ($s, $result) = @_;
  my $executor = $s->tasks->[$result->itemid]->item;
  if ($executor->can('e') and $executor->e) {
    $result->name($executor->e->eid);
  }
  if ($executor->can('handleResult')) {
    my $rc = $executor->handleResult($s, $result);
    if ($rc == RETURN_AND_REPORT) { $s->return_and_report($result) }
  }
  else {
    $s->return_and_report($result);
  }
}

sub schedule_at {
  my ($s, $result, $runtime) = @_;
  my $task = $s->task($result->itemid);
  $task->runtime($runtime);
  $s->return_node_resources($task);
  push @{ $s->waitTasks }, $task;
}

sub schedule {
  my ($s) = @_;
  my $new_wait_list = [];

  #iterate through waiting tasks adding ready tasks to runqueue
  for (@{$s->waitTasks}) {
    my $id = $_->id;
    if ($_->ready) { push @{$s->runqueue}, $_; }
    else { push @$new_wait_list, $_; }  
  }
  $s->sort_runqueue;
  $s->waitTasks($new_wait_list);
  return (scalar @$new_wait_list) || scalar (@{$s->runqueue});
}

sub retry {
  my ($s, $result) = @_;
  my $itemid = $result->itemid;
  if (!$s->inRetry) {
    push @{ $s->retryTasks }, $s->task($itemid);
    $s->return_node_resources($s->task($itemid));
#    say "RETRYING task# $itemid";
    return 1;
  }
  else { 
#    say "DONE RETRYING";
    $s->return_and_report($result); 
  }
}

1;
