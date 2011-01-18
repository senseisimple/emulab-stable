#!/usr/bin/perl
#
use English;
use Getopt::Std;
use Time::HiRes qw( gettimeofday tv_interval );

#
# This script is for load testing boss; not intended for user use. 
#
sub usage()
{
    print STDERR "partest.pl [-n count] [operation]\n";
    exit(1);
}
my $optlist  = "n:";
my $COUNT    = 1;
my $op       = "getcredential";
my @names    = ();

# Protos
sub Run();
sub ParRun($$$@);

#
# Parse command arguments.
#
my %options = ();
if (! getopts($optlist, \%options)) {
    usage();
}
if (@ARGV > 1) {
    usage();
}
if (defined($options{"n"})) {
    $COUNT = $options{"n"};
}
$op = $ARGV[0]
    if (@ARGV);

for (my $i = 0; $i < $COUNT; $i++) {
    push(@names, "lbstest$i");
}

Run();
exit(0);

sub Run()
{
    my $script;
    my $args = "";
    if ($op eq "getcredential") {
	$script = "getcredential.py";
    }
    elsif ($op eq "getslicecredential") {
	$script = "getslicecredential.py";
    }
    elsif ($op eq "register") {
	$script = "registerslice.py";
    }
    elsif ($op eq "getticket") {
	$script = "getticket.py";
	$args   = "jailnode.rspec";
#	$args   = "gec.rspec";
    }
    elsif ($op eq "redeem") {
	$script = "redeemticket.py";
    }
    elsif ($op eq "createsliver") {
	$script = "createsliver.py";
	$args   = "jailnode.rspec";
#	$args   = "gec.rspec";
    }
    elsif ($op eq "releaseticket") {
	$script = "releaseticket.py";
    }
    elsif ($op eq "start") {
	$script = "sliveraction.py";
	$args   = "start";
    }
    elsif ($op eq "status") {
	$script = "sliverstatus.py";
    }
    elsif ($op eq "renew") {
	$script = "renewsliver.py";
	$args   = "100";
    }
    elsif ($op eq "delete") {
	$script = "deleteslice.py";
    }
    else {
	print STDERR "What do I do with $op\n";
	exit(1);
    }
    
    my @results   = ();
    my $coderef   = sub {
	my ($slicename) = @_;

	print STDERR "$slicename: Running $op $args\n";
	my $starttime = [gettimeofday()];
	system("$script -n $slicename $args") == 0
	    or return -1;

	my $elapsed = tv_interval($starttime);
	print STDERR "$slicename: Elapsed time: " . sprintf("%f\n", $elapsed);
	return 0;
    };
    if (ParRun({'maxwaittime' => 600}, \@results, $coderef, @names)) {
	print STDERR "*** Internal error doing $op\n";
	exit(1);
    }
    #
    # Check the exit codes. 
    #
    my $errors   = 0;
    my $count    = 0;
    foreach my $result (@results) {
	my $slicename = @names[$count];

	if ($result != 0) {
	    print STDERR "*** Error running op $slicename\n";
	    $errors++;
	}
	$count++;
    }
    return $errors;
}

sub ParRun($$$@)
{
    my ($options, $pref, $function, @objects) = @_;
    my %children = ();
    my @results  = ();
    my $counter  = 0;
    my $signaled = 0;

    # options.
    my $maxchildren = 10;
    my $maxwaittime = 200;

    if (defined($options)) {
	$maxchildren = $options->{'maxchildren'}
	    if (exists($options->{'maxchildren'}));
	$maxwaittime = $options->{'maxwaittime'}
	    if (exists($options->{'maxwaittime'}));
    }

    #
    # Set up a signal handler in the parent to handle termination.
    #
    my $coderef = sub {
	my ($signame) = @_;

	print STDERR "Caught SIG${signame}! Killing parrun ...";

	$SIG{TERM} = 'IGNORE';
	$signaled = 1;

	foreach my $pid (keys(%children)) {
	    kill('TERM', $pid);
	}
	sleep(1);
    };
    local $SIG{QUIT} = $coderef;
    local $SIG{TERM} = $coderef;
    local $SIG{HUP}  = $coderef;
    local $SIG{INT}  = 'IGNORE';

    #
    # Initialize return.
    #
    for (my $i = 0; $i < scalar(@objects); $i++) {
	$results[$i] = -1;
    }

    while (@objects || keys(%children)) {
	#
	# Something to do and still have free slots.
	#
	if (@objects && keys(%children) < $maxchildren && !$signaled) {
	    # Space out the invocation of child processes a little.
	    sleep(2);
	    
	    #
	    # Run command in a child process, protected by an alarm to
	    # ensure that whatever happens is not hung up forever in
	    # some funky state.
	    #
	    my $object = shift(@objects);
	    my $syspid = fork();

	    if ($syspid) {
		#
		# Just keep track of it, we'll wait for it finish down below
		#
		$children{$syspid} = [$object, $counter, time()];
		$counter++;
	    }
	    else {
		$SIG{TERM} = 'DEFAULT';
		$SIG{QUIT} = 'DEFAULT';
		$SIG{HUP}  = 'DEFAULT';
		
		if ($EVENTSYS) {
		    # So we get the event system fork too ...
		    EventFork();
		}
		exit(&$function($object));
	    }
	}
	elsif ($signaled) {
	    my $childpid   = wait();
	    my $exitstatus = $?;

	    if (exists($children{$childpid})) {
		delete($children{$childpid});
	    }
	}
	else {
	    #
	    # We have too many of the little rugrats, wait for one to die
	    #
	    #
	    # Set up a timer - we want to kill processes after they
	    # hit timeout, so we find the first one marked for death.
	    #
	    my $oldest;
	    my $oldestpid = 0;
	    my $oldestobj;
	    
	    while (my ($pid, $aref) = each %children) {
		my ($object, $which, $birthtime) = @$aref;

		if ((!$oldestpid) || ($birthtime < $oldest)) {
		    $oldest    = $birthtime;
		    $oldestpid = $pid;
		    $oldestobj = $object;
		}
	    }

	    #
	    # Sanity check
	    #
	    if (!$oldest) {
		print STDERR 
		    "*** ParRun: ".
		    "Uh oh, I have no children left, something is wrong!\n";
	    }

	    #
	    # If the oldest has already expired, just kill it off
	    # right now, and go back around the loop
	    #
	    my $now = time();
	    my $waittime = ($oldest + $maxwaittime) - time();

	    #
	    # Kill off the oldest if it gets too old while we are waiting.
	    #
	    my $childpid = -1;
	    my $exitstatus = -1;

	    eval {
		local $SIG{ALRM} = sub { die "alarm clock" };

		if ($waittime <= 0) {
		    print STDERR
			"*** ParRun: timeout waiting for child: $oldestpid\n";
		    kill("TERM", $oldestpid);
		}
		else {
		    alarm($waittime);
		}
		$childpid = wait();
		alarm 0;
		$exitstatus = $?;
	    };
	    if ($@) {
		die unless $@ =~ /alarm clock/;
		next;
	    }

	    #
	    # Another sanity check
	    #
	    if ($childpid < 0) {
		print STDERR
		    "*** ParRun:\n".
		    "wait() returned <0, something is wrong!\n";
		next;
	    }

	    #
	    # Look up to see what object this was associated with - if we
	    # do not know about this child, ignore it
	    #
	    my $aref = $children{$childpid};
	    next unless @$aref;	
	    my ($object, $which, $birthtime) = @$aref;
	    delete($children{$childpid});
	    $results[$which] = $exitstatus;
	}
    }
    @$pref = @results
	if (defined($pref));
    return -1
	if ($signaled);
    return 0;
}
