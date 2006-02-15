#!/usr/bin/perl -w
#
# Process Emulab log mail
#
# TODO
#   Sharks were alloc'ed/dealloc'ed in shelf units, account for this
#   Add check for node names pc600/pc850/etc. to make sure we don't misparse
#   Differentiate BATCH{IN,OUT} create/terminate from swapin/swapout
#

use Getopt::Std;
use POSIX;
use tbmail;

sub usage()
{
    print "Usage: parsemail [-CSWdh] < maillog > records\n".
	  "  Generate experiment records from a maillog.".
	  "Options:\n".
          "  -C      don't check date and time stamps for sanity\n".
          "  -S      don't sort node lists in output records\n".
          "  -W      don't whine about problems in messages\n".
          "  -d      print (lots of!) debug info\n".
	  "  -h      print this help message\n";
    exit(1);
}
my $optlist = "CSWdf";

my $fixup = 0;
my $debug = 0;
my $whine = 1;
my $chkdate = 1;
my $sortem = 1;

my $msgs = 0;
my $ignored = 0;
my @curmsg = ();
my %seen;
my @records = ();

#
# Regular expressions for matching various message lines.
# RE arrays starting with '_' get compiled into the non-'_' versions for
# efficiency.
#

#
# Experiment related REs and patterns
#
my $PID_RE = '[-.\w]+';
my $EID_RE = '[-.\w]+';
my $UID_RE = '[a-zA-Z][-\w]+';
my $TB_RE = '^(TESTBED|EMULAB\.NET): ';

my @create_info = (
    {
	_pat => "New Experiment Created: ($PID_RE)\/($EID_RE)",
	action => CREATE2,
    },
    {
	_pat => "New Experiment Started: ($PID_RE)\/($EID_RE)",
	action => CREATE2,
    },
    # ancient history
    {
	_pat => "'($PID_RE)' '($EID_RE)' New Experiment Created",
	action => CREATE1,
    },
    {
	_pat => 'New Experiment Created$',
	action => CREATE1,
    },
);

my $SWAPPED_RE = '[Ss]wapped';

my @_swapin_pat = (
  "Experiment ($PID_RE)\/($EID_RE) $SWAPPED_RE in"
);
my @swapin_pat;
my @_swapout_pat = (
  "Experiment ($PID_RE)\/($EID_RE) $SWAPPED_RE out"
);
my @swapout_pat;
my @_forcedswapout_pat = (
  "idleswap (?:-r )?(-[ai] )?($PID_RE) ($EID_RE)"
);
my @forcedswapout_pat;
my @_terminate_pat = (
  "Experiment ($PID_RE)\/($EID_RE) Terminated"
);
my @terminate_pat;
my @_batch_pat = (
  "Batch Mode Experiment Status ($PID_RE)\/($EID_RE)",
  "Batch Mode Experiment Canceled ($PID_RE)\/($EID_RE)",
  "Batch Mode Cancelation ($PID_RE)\/($EID_RE)",
);
my @batch_pat;
my @_preload_pat = (
  "New Experiment Preloaded: ($PID_RE)\/($EID_RE)",
);
my @preload_pat;
my @_modify_pat = (
  "Experiment ($PID_RE)\/($EID_RE) [Mm]odified",
);
my @modify_pat;
my @_modifyfail_pat = (
  "Swap modify Failure: ($PID_RE)\/($EID_RE)",
);
my @modifyfail_pat;

#
# Node related REs and patterns
#
my $PC_RE = '(?:tb)?pc\d+';
my $SHARK_RE = 'sh\d+\-\d+|tbsh\d+';
my $RON_RE = 'vron\d+';
my $NODE_RE = "(?:$PC_RE|$SHARK_RE|$RON_RE)";

#
# "alive and well" may not be at beginning of line due to parallel
# operation of snmpit, it may be generating output at the same time.
# It should always be the last thing on the line however.
#
my @_nodealloc_pat = (
  "($NODE_RE) is alive and well\$",
  "^Not waiting for ($NODE_RE) to come alive\. Foreign OS\."
);
my @nodealloc_pat;

my @_nodefree_pat = (
  "^Releasing node '($NODE_RE)'",
  "^Changing reservation for ($NODE_RE) to testbed/",
  "^Changing reservation for ($NODE_RE) to emulab-ops/",
  "^Moving ($NODE_RE) to emulab-ops/",
);
my @nodefree_pat;

# fwd message patterns that otherwise match TB_RE
my @fwd_pat = (
  '\(fwd\)$',
);

#
# Things we don't care about right now
# Note: TB_RE prefix has been stripped from string before this match.
#
my @_ignore_pat = (
  '^Account',
  '^DBError',
  '^Project',
  '^Group',
  '^User',
  '^Image Creation',
  '^Node Update',
  '^mkacct ',
  '^.* WA node ',
);
my @ignore_pat;

#
# Compile all the variable interpolating patterns for efficiency.
#
sub compilepatterns()
{
    # subject lines
    map { $_->{pat} = $_->{_pat} } @create_info;
    @swapin_pat = map { qr/$_/ } @_swapin_pat;
    @swapout_pat = map { qr/$_/ } @_swapout_pat;
    @forcedswapout_pat = map { qr/$_/ } @_forcedswapout_pat;
    @terminate_pat = map { qr/$_/ } @_terminate_pat;
    @batch_pat = map { qr/$_/ } @_batch_pat;
    @preload_pat = map { qr/$_/ } @_preload_pat;
    @modify_pat = map { qr/$_/ } @_modify_pat;
    @modifyfail_pat = map { qr/$_/ } @_modifyfail_pat;
    @ignore_pat = map { qr/$_/ } @_ignore_pat;

    # nodes
    @nodealloc_pat = map { qr/$_/ } @_nodealloc_pat;
    @nodefree_pat = map { qr/$_/ } @_nodefree_pat;
}

sub NODE_ALLOC()     { return 1 };
sub NODE_DEALLOC()   { return 2 };
sub NODE_UNKNOWN()   { return 3 };

sub STATE_NORMAL()   { return 1 };
sub STATE_OCREATE()  { return 2 };	# 2000-ish create message
sub STATE_MODIFY()   { return 3 };

sub STATE_BATCH()            { return 10 };	# inside a BATCH message
sub STATE_BATCHCREATE()      { return 11 };	# have generated a BATCHCREATE
sub STATE_BATCHCREATEMAYBE() { return 12 };	# might have seen BATCHCREATE
sub STATE_BATCHTERM()        { return 13 };	# have generated a BATCHTERM
sub STATE_BATCHTERMMAYBE()   { return 14 };	# might have seen BATCHTERM
sub STATE_BATCHPRELOAD()     { return 15 };	# did prerun, not a SWAPIN
sub IN_BATCHSTATE($)
    { my $s = shift; return $s >= STATE_BATCH && $s <= STATE_BATCHPRELOAD; }

# Node List processing
sub STATE_NLSEARCH()	  { return 20 };	# looking for start of NL
sub STATE_NLSTARTED()	  { return 21 };	# processing node list
sub IN_NLSTATE($)
    { my $s = shift; return $s >= STATE_NLSEARCH && $s <= STATE_NLSTARTED; }

sub DATE_MBOX()    { return 1 };
sub DATE_CTIME()   { return 2 };

sub processmsg(@);
sub parsedate($$);
sub parsesubject($);
sub parsenode($);
sub parsebatch($$$);
sub createrecord(%);
sub printmsg(%);

#
# Parse command arguments.
#
%options = ();
if (! getopts($optlist, \%options)) {
    usage();
}
if (defined($options{"C"})) {
    $chkdate = 0;
}
if (defined($options{"S"})) {
    $sortem = 0;
}
if (defined($options{"W"})) {
    $whine = 0;
}
if (defined($options{"d"})) {
    $debug = 1;
}
if (defined($options{"f"})) {
    $fixup = 1;
}

compilepatterns();

while ($line = <STDIN>) {
    #
    # Found the start of a message.
    # Process the message that just ended before starting afresh
    #
    if ($line =~ /^From /) {
	$msgs++;
	processmsg(@curmsg)
	    if (@curmsg);
	@curmsg = ();
    }
    chomp($line);
    push(@curmsg, $line);
}
processmsg(@curmsg)
    if (@curmsg);

#
# Sort the generated messages and eliminate records with bad (0) dates
#
if (@records > 0) {
    @records = sortrecords(@records);
    while ($records[0][REC_STAMP] == 0) {
	shift @records;
	$ignored++;
    }
}

#
# Print out experiment records
#
for my $rec (@records) {
    my ($stamp, $pid, $eid, $uid, $action, $msgid, @nlist) = @$rec;
    printrecord([$stamp, $pid, $eid, $uid, $action, $msgid, @nlist], $sortem);
}

#
# and summarize.
#
print STDERR "Processed $msgs messages";
print STDERR " ($ignored ignored)"
    if ($ignored);
print STDERR ", ", scalar(@records), " records created";
print STDERR "\n";

exit(0);

#
# Process a single mail message.  Extract all the information we need,
# and create one or more experiment records.
#
sub processmsg(@) {
    my (@lines) = @_;

    my %msg = ();
    $msg{action} = BAD;
    $msg{nodes} = {};

    my $state = STATE_NORMAL;
    for my $line (@lines) {
	if ($line =~ /^Date: (.*)/) {
	    $msg{stamp} = parsedate($1, DATE_MBOX)
		if (!$msg{stamp});
	    next;
	}
	if ($line =~ /^Subject: (.*)/) {
	    my $subject = $1;

	    #
	    # Got a second subject line.  In a legit message, this is a
	    # serious error so we always complain.  For other messages,
	    # it doesn't matter because the message will not result in
	    # a record anyway.  Just make sure we don't modify any state.
	    #
	    if ($msg{subject}) {
		if ($msg{action} != BAD) {
		    print STDERR "*** $msg{id}: multiple subject lines\n".
			         "    Original subject: $msg{subject}\n";
		}
	    } else {
		($msg{action}, $msg{pid}, $msg{eid}) = parsesubject($subject);

		# XXX mini did not always properly identify itself
		if ($msg{id} && ($msg{id} =~ /mini\.emulab\.net/)) {
		    $msg{pid} = undef;
		    $msg{eid} = undef;
		    $msg{action} = BAD;
		    last;
		}

		if ($msg{action} == BATCH) {
		    $state = STATE_BATCH;
		} elsif ($msg{action} == CREATE1) {
		    $state = STATE_OCREATE;
		}
		#
		# Prefer the summary section for extracting node info
		#
		elsif ($msg{action} == CREATE2 || $msg{action} == SWAPIN) {
		    $state = STATE_NLSEARCH;
		    $msg{nodes} = {};
		}
		#
		# Handle a modify message
		# Right now, I don't want to mess with distinguishing what nodes
		# get alloced and freed by parsing the action log, so we just
		# parse the resulting node list and calculate from that.
		#
		elsif ($msg{action} == MODIFY) {
		    $state = STATE_NLSEARCH;
		    $msg{nodes} = {};
		}
		#
		# For an idleswap message, we have everything we need by
		# the time we parse the subject line.
		#
		elsif ($msg{action} == IDLESWAPOUT ||
		       $msg{action} == AUTOSWAPOUT ||
		       $msg{action} == FORCEDSWAPOUT) {
		    if (!$msg{stamp} || !$msg{pid} || !$msg{eid} ||
			!$msg{uid} || !$msg{id}) {
			print STDERR
			    "*** Bad idleswap message $msg{id} ignored\n"
				if ($whine);
			return;
		    }
		    $msg{subject} = $subject;
		    last;
		}
		$msg{subject} = $subject;
	    }
	    next;
	}
	if ($line =~ /^Message-[Ii][Dd]: (<.*>)$/) {
	    if (!$msg{id}) {
		$msg{id} = $1;
		if ($seen{$msg{id}}) {
		    print STDERR "*** Duplicate message $msg{id} ignored\n"
			if ($whine);
		    return;
		}
		$seen{$msg{id}} = 1;
	    }
	    next;
	}

	#
	# XXX return path is the most accurate indicator of the user as
	# they are known inside Emulab (the header 'From:' line has the
	# name of the user in their home environment, which may not be
	# the same as their Emulab name).  However, this will return "nobody"
	# for records in the 2000 to early 2001 time frame.  For those we
	# try to parse out a 'User:' line from the message body (see below).
	#
	if ($line =~ /^Return-Path: <($UID_RE)\@/o) {
	    $msg{uid} = $1
		if ($1 ne "nobody");
	    next;
	}

	#
	# Well, in 2004 we changed the mailer on bas so it reports the
	# return path as "mailnull" so we dig a little deeper
	#
	if ($msg{uid} && $msg{uid} eq "mailnull" &&
	    $line =~ /\(envelope-from ($UID_RE)\@boss\.emulab\.net\)/o) {
	    $msg{uid} = $1;
	}

	#
	# A failed modify can result in a swapout
	#
	if ($msg{action} == SWAPOUTMAYBE &&
	    $line =~ /Swapping experiment out\./) {
	    $msg{action} = SWAPOUT;
	}

	#
	# Ok, its not just early create messages that need this.  There
	# are some 2002-2003 messages as well, so we pull it out here
	# doing the best we can to make sure we only match a 'User'
	# record in the body...
	#
	# For these early create messages, we also grab user name records
	# if we don't already have a valid uid from the return-path.
	# This will leave us without a valid uid for terminate, swap, etc.
	# messages, but we aren't likely to care about a uid in those cases.
	# Note the anchoring '$' in this case, this is to prevent matching
	# full names which also appeared in some early records.
	#
	if (!$msg{uid} && $msg{subject} && $line =~ /^User:\s+($UID_RE)$/o) {
	    $msg{uid} = $1;
	}

	#
	# Handle 2000-ish create messages where pid/eid not in the
	# subject line.  We do this before matching NODE_RE so we can
	# be more careful matching the old style assigned node output.
	#
	if ($state == STATE_OCREATE) {
	    if ($line =~ /^PID:\s+($PID_RE)/o) {
		$msg{pid} = $1;
	    } elsif ($line =~ /^EID:\s+($EID_RE)/o) {
		$msg{eid} = $1;
	    } elsif ($line =~ /\s\s+($NODE_RE)\s\s+/o) {
		my $node = $1;
		# Map historical PC names to current names
		if ($node =~ /tb(pc.*)/) {
		    ($node = $1) =~ s/pc0/pc/;
		}
		# and sharks
		elsif ($node =~ /tb(sh\d+)/) {
		    ($node = $1) =~ s/$/-1/;
		}
		$msg{nodes}{$node} = NODE_ALLOC;
	    }
	    next;
	}

	#
	# XXX there have been differents forms of this, but we only
	# care about the one used since experiment modify came about
	#
	if (IN_NLSTATE($state)) {
	    if ($state == STATE_NLSTARTED) {
		#
		# Got the goods, we are done with this message.
		# Don't keep going or we might start parsing the
		# experiment log.
		#
		if ($line =~ /Lan\/Link Info:$/) {
		    $state = STATE_NORMAL;
		    last;
		}
		#
		# Ditto.  Not all experiments have Lan/Link Info.
		# Maybe just use this for all?
		#
		elsif ($line =~ /^Beginning pre run for/) {
		    $state = STATE_NORMAL;
		    last;
		}
		if ($line =~ /\s+($NODE_RE)$/o) {
		    my $node = $1;
		    # Map historical PC names to current names
		    if ($node =~ /tb(pc.*)/) {
			($node = $1) =~ s/pc0/pc/;
		    }
		    # and sharks
		    elsif ($node =~ /tb(sh\d+)/) {
			($node = $1) =~ s/$/-1/;
		    }
		    $msg{nodes}{$node} = NODE_UNKNOWN;
		}
		#
		# Eeewww...ron nodes make our life miserable
		#
		elsif ($line =~ /\s+($RON_RE) \(ron\d+\)$/o) {
		    $msg{nodes}{$1} = NODE_UNKNOWN;
		}
	    } else {
		if ($line =~ /^Physical Node Mapping:$/) {
		    $state = STATE_NLSTARTED;
		}
		#
		# Record whether there are any virtual nodes.
		#
		elsif ($line =~ /^Virtual Node Info:$/) {
		    $msg{vnodes} = 1;
		}

		#
		# Hit the end of the summary section without match
		# fall back on regular parsing (and yes it really
		# is "swapin in").
		#
		elsif ($line =~ /^Beginning (?:pre run|swapin in|swap-in) for/) {
#		elsif ($line =~ /^Beginning pre run for/ || $line =~ /^Beginning swapin in for/) {
		    $state = STATE_NORMAL;
		}
	    }
	    next;
	}

	#
	# Recognize lines that represent a node being allocated or freed
	#
	if ($line =~ /$NODE_RE/o) {
	    my ($node,$alloc) = parsenode($line);

	    if ($node) {
		#
		# Check for nodes changing allocation status
		#
		if (exists($msg{nodes}{$node})) {
		    # ALLOC -> ALLOC and ALLOC -> DEALLOC are ok
		    if ($msg{nodes}{$node} != NODE_ALLOC) {
			print STDERR "Found $node twice in ", $msg{id}, ": ",
			             $msg{nodes}{$node}, "->$alloc\n";
		    }
		}
		$msg{nodes}{$node} = $alloc;
	    }
	    next;
	}

	#
	# Handle BATCH messages which could include both create/terminate.
	# The remainder of the message is processed here and may produce
	# multiple records.  No further processing is done.
	#
	if (IN_BATCHSTATE($state)) {
	    my ($action, $stamp) = parsebatch($line, $msg{stamp}, $state);
	    my $ostate = $state
		if ($debug);
	    if ($action == BATCHPRELOAD) {
		# only the first thing
		# XXX may also follow a suspected terminate message
		if ($state == STATE_BATCH) {
		    $msg{action} = $action;
		    $state = STATE_BATCHPRELOAD;
		}
	    } elsif ($action == BATCHCREATE || $action == BATCHSWAPIN) {
		# ignore unless it is the first part of the message
		if ($state == STATE_BATCH ||
		    $state == STATE_BATCHCREATEMAYBE ||
		    $state == STATE_BATCHPRELOAD ||
		    $state == STATE_BATCHTERMMAYBE) {
		    $msg{action} = $action;
		    $msg{stamp} = $stamp;
		    createrecord(%msg);
		    $state = STATE_BATCHCREATE;
		}
	    } elsif ($action == BATCHCREATEMAYBE) {
		# can only be first
		if ($state == STATE_BATCH ||
		    $state == STATE_BATCHPRELOAD) {
		    $msg{action} = $action;
		    $msg{stamp} = $stamp;
		    $state = STATE_BATCHCREATEMAYBE;
		}
	    } elsif ($action == BATCHTERM || $action == BATCHSWAPOUT) {
		# can be first, follow a BATCHCREATE or a BATCHTERMMAYBE
		if ($state == STATE_BATCH ||
		    $state == STATE_BATCHCREATE ||
		    $state == STATE_BATCHTERMMAYBE) {
		    $msg{action} = $action;
		    $msg{stamp} = $stamp;
		    createrecord(%msg);
		    $state = STATE_BATCHTERM;
		}
	    } elsif ($action == BATCHTERMMAYBE) {
		# can only be first
		if ($state == STATE_BATCH) {
		    $msg{action} = $action;
		    $msg{stamp} = $stamp;
		    $state = STATE_BATCHTERMMAYBE;
		}
	    } elsif ($action == BATCHIGNORE) {
		# ignore this message entirely
		$state = STATE_NORMAL;
		$msg{action} = BAD;
		last;
	    }
	    print STDERR "Batch: state $ostate->$state, action $action for:\n".
		         "  $msg{id}\n '$line'\n"
			 if ($debug && $action != BAD);
	    next;
	}
    }

    if (IN_BATCHSTATE($state)) {
	#
	# XXX batch mode hack
	# See 2002 comment in parsebatchmode
	#
	if ($state == STATE_BATCHCREATEMAYBE) {
	    $msg{action} = BATCHCREATE;
	    $state = STATE_BATCHCREATE;
	}
	#
	# XXX batch mode hack
	# circa late 2002 batch termination messages included just a message
	# saying the job was finished.  Unfortunately, that message also shows
	# up at the beginning of batch messages which do contain more info.
	# So we don't want to commit to a BATCHTERMMAYBE unless we know there
	# is not more, better info to come.
	#
	elsif ($state == STATE_BATCHTERMMAYBE) {
	    $msg{action} = BATCHTERM;
	    $state = STATE_BATCHTERM;
	}
	# Mark as BAD so we don't create another record
	else {
	    $msg{action} = BAD;
	}
    }

    #
    # If this was a create message with virtual nodes but not physical nodes
    # then it must be a PRELOAD rather than a CREATE.
    #
    if ($msg{vnodes} && keys(%{$msg{nodes}}) == 0 && ISCREATE($msg{action})) {
	$msg{action} = PRELOAD;
    }

    #
    # Modify failure that recovered and didn't swapout
    #
    if ($msg{action} == SWAPOUTMAYBE) {
	$msg{action} = BAD;
    }

    #
    # If not a BAD message, create a record for it
    #
    if ($msg{action} != BAD) {
	createrecord(%msg);
    }
    #
    # Otherwise mark as ignored (unless it was a processed BATCH message)
    #
    elsif (!IN_BATCHSTATE($state)) {
	$msg{msgid} = "<UNKNOWN>"
	    if (!$msg{msgid});
	print STDERR "Ignored: '$msg{subject}'\n"
	    if ($whine);
	$ignored++;
    }
}

sub createrecord(%) {
    my (%msg) = @_;

    printmsg(%msg)
	if ($debug > 1);

    #
    # Experiments may replace nodes that fail during a swapin,
    # so be on the lookout for node deallocation messages in the
    # middle of swapin/batchswapin.  We just remove such nodes
    # entirely from this operation.
    #
    if ($msg{action} == SWAPIN || $msg{action} == BATCHSWAPIN) {
	while (my ($node,$alloc) = each(%{$msg{nodes}})) {
	    if ($alloc == NODE_DEALLOC) {
		print STDERR "$node deallocated in ", ACTIONSTR($msg{action}),
                             " msg ", $msg{id}, "\n";
		delete($msg{nodes}{$node});
	    }
	}
    }

    # save an experiment record
    if ($msg{pid}) {
	$msg{uid} = "nobody"
	    if (!$msg{uid});
	my $rec = makerecord($msg{stamp}, $msg{pid}, $msg{eid}, $msg{uid},
			     $msg{action}, $msg{id}, keys(%{$msg{nodes}}));
	push(@records, $rec);
    }
}

sub printmsg(%) {
    my (%msg) = @_;

    my $mdate = ctime($msg{stamp});
    chomp($mdate);

    print STDERR "$mdate: $msg{action}: ";
    print STDERR "$msg{pid}/$msg{eid}: "
	if ($msg{pid});
    my @nlist = keys(%{$msg{nodes}});
    print STDERR "nodes=(" . join(",",@nlist) . "), "
	if (@nlist > 0);
    print STDERR "\n";
}

#
# Convert time-only stamp into unix timestamp based on a reference timestamp
# from the message header.  Note: the message stamp is larger than the expected
# date.  We are assuming that the message content is within 24 hours of the
# message being sent.
#
sub parsetime($$) {
    my ($tstr, $refstamp) = @_;
    my ($hour, $min, $sec);
    my $stamp;

    ($hour, $min, $sec) = split(":", $tstr);

    ($rsec,$rmin,$rhour,$rmday,$rmon,$ryear,$rwday,$ryday,$risdst) =
	localtime($refstamp);

    my $dsec = ($hour * 60 + $min) * 60 + $sec;
    my $rdsec = ($rhour * 60 + $rmin) * 60 + $rsec;

    # easy: no wrap to the previous day
    if ($dsec <= $rdsec) {
	$stamp = mktime($sec, $min, $hour, $rmday, $rmon, $ryear,
			0, 0, $risdst);
    }
    # otherwise, back reference date to the previous day and try again
    else {
	$stamp = $refstamp - ($rdsec + 1);
	($rsec,$rmin,$rhour,$rmday,$rmon,$ryear,$rwday,$ryday,$risdst) =
	    localtime($stamp);
	$rdsec = ($rhour * 60 + $rmin) * 60 + $rsec;
	if ($dsec > $rdsec || $rhour != 23 || $rmin != 59 || $rsec != 59) {
	    print STDERR "Cannot calculate date for '$tstr' in msg #$msgs\n"
		if ($whine);
	    return $refstamp;
	}
	$stamp = mktime($sec, $min, $hour, $rmday, $rmon, $ryear,
			0, 0, $risdst);
    }
    if ($chkdate) {
	my $dstr = ctime($stamp);
	chomp($dstr);

	if ($dstr =~ /^(\w+) (\w+)  ?(\d+) ([\d:]+) (\d+)$/) {
	    ($rhour, $rmin, $rsec) = split(":", $4);
	} else {
	    $rhour = -1;
	}
	if ($rhour != $hour || $rmin != $min || $rsec != $sec) {
	    print STDERR "Computed date '$dstr' does not match time '$tstr'\n"
		if ($whine);
	    return $refstamp;
	}
    }
    return $stamp;
}

#
# Convert mbox or ctime style date into unix timestamp
#
sub parsedate($$) {
    my ($str, $style) = @_;

    my %mnum = ('Jan'=>1,'Feb'=>2,'Mar'=>3,'Apr'=>4,
		'May'=>5,'Jun'=>6,'Jul'=>7,'Aug'=>8,
		'Sep'=>9,'Oct'=>10,'Nov'=>11,'Dec'=>12);
    my %dnum = ('Sun'=>1,'Mon'=>2,'Tue'=>3,'Wed'=>4,
		'Thu'=>5,'Fri'=>6,'Sat'=>7);
    my ($wday, $day, $month, $year, $hour, $min, $sec, $isdst);
    my ($tstr, $dstr, $mstr, $tzstr);

    if ($style == DATE_MBOX &&
	$str =~ /^([^,]+), (\d+) (\w+) (\d+) ([\d:]+) ([-\d]+) \((\w+)\)$/) {
	$dstr = $1;
	$day = $2;
	$mstr = $3;
	$year = $4;
	$tstr = $5;
	$tzstr = $7;
    }
    elsif ($style == DATE_CTIME &&
	   $str =~ /^(\w+) (\w+)  ?(\d+)  ?([\d:]+) (\d+)$/) {
	$dstr = $1;
	$mstr = $2;
	$day = $3;
	$tstr = $4;
	$year = $5;
	$tzstr = "MST"; # XXX will be caught later
    }
    else {
	print STDERR "Bad msg date '$str' in msg #$msgs\n"
	    if ($whine);
	return 0;
    }

    if ($dnum{$dstr}) {
	$wday = $dnum{$dstr} - 1;
    } else {
	print STDERR "Bad msg date day '$dstr' in msg #$msgs\n"
	    if ($whine);
	return 0;
    }
    if ($mnum{$mstr}) {
	$month = $mnum{$mstr} - 1;
    } else {
	print STDERR "Bad msg date month '$mstr' in msg #$msgs\n"
	    if ($whine);
	return 0;
    }
    $year -= 1900;
    ($hour, $min, $sec) = split(":", $tstr);
    if ($tzstr eq "MST") {
	$isdst = 0;
    } elsif ($tzstr eq "MDT") {
	$isdst = 1;
    } else {
	print STDERR "Bad msg date timezone '$tzstr' in msg #$msgs\n"
	    if ($whine);
	return 0;
    }

    my $stamp;
    while (1) {
	$stamp = mktime($sec, $min, $hour, $day, $month, $year,
			   0, 0, $isdst);
	if ($chkdate) {
	    my $ystr = $year + 1900;
	    $day = " $day" if ($day =~ /^\d$/);
	    $hour = "0$hour" if ($hour =~ /^\d$/);
	    my $odate = "$dstr $mstr $day $hour:$min:$sec $ystr";
	    my $ndate = ctime($stamp);
	    chomp($ndate);
	    if ($odate ne $ndate) {
		if ($style == DATE_CTIME && $isdst == 0) {
		    $isdst = 1;
		    next;
		}
		print STDERR "Bad date conversion: '$odate' to '$ndate'\n";
		return 0;
	    }
	}
	last;
    }

    return $stamp;
}

sub parsesubject($)
{
    my ($str) = @_;
    my $pat;

    # must start with the magic string
    if ($str !~ /$TB_RE/) {
	print STDERR "Non-testbed generated subject line: '$str'\n"
	    if ($whine);
	return (BAD, undef, undef);
    }
    $str =~ s/$TB_RE//;

    # get rid of forwarded messages
    for $pat (@fwd_pat) {
	if ($str =~ /$pat/) {
	    return (BAD, undef, undef);
	}
    }

    # now look for things we care about
    for $info (@create_info) {
	if ($str =~ /$info->{pat}/) {
	    return ($info->{action}, $1, $2);
	}
    }
    for $pat (@swapin_pat) {
	if ($str =~ /$pat/) {
	    return (SWAPIN, $1, $2);
	}
    }
    for $pat (@swapout_pat) {
	if ($str =~ /$pat/) {
	    return (SWAPOUT, $1, $2);
	}
    }
    for $pat (@forcedswapout_pat) {
	if ($str =~ /$pat/) {
	    if (!defined($1)) {
		return (FORCEDSWAPOUT, $2, $3);
	    }
	    if ($1 eq "-i ") {
		return (IDLESWAPOUT, $2, $3);
	    }
	    return (AUTOSWAPOUT, $2, $3);
	}
    }
    for $pat (@terminate_pat) {
	if ($str =~ /$pat/) {
	    return (TERMINATE, $1, $2);
	}
    }
    for $pat (@batch_pat) {
	if ($str =~ /$pat/) {
	    return (BATCH, $1, $2);
	}
    }
    for $pat (@preload_pat) {
	if ($str =~ /$pat/) {
	    return (PRELOAD, $1, $2);
	}
    }
    for $pat (@modify_pat) {
	if ($str =~ /$pat/) {
	    return (MODIFY, $1, $2);
	}
    }
    for $pat (@modifyfail_pat) {
	if ($str =~ /$pat/) {
	    return (SWAPOUTMAYBE, $1, $2);
	}
    }

    # things we explicitly want to ignore (for now)
    for $pat (@ignore_pat) {
	if ($str =~ /$pat/) {
	    return (IGNORE, undef, undef);
	}
    }

    return (BAD, undef, undef);
}

#
# Attempt to parse a message line involving a node.
# We look for either node allocations or node deallocations.
#
sub parsenode($) {
    my ($str) = @_;
    my $pat;
    my $alloc;

    my $node = "";
    for $pat (@nodealloc_pat) {
	if ($str =~ /$pat/) {
	    $node = $1;
	    $alloc = NODE_ALLOC;
	    last;
	}
    }
    if ($node eq "") {
	for $pat (@nodefree_pat) {
	    if ($str =~ /$pat/) {
		$node = $1;
		$alloc = NODE_DEALLOC;
		last;
	    }
	}
    }
    # compensate for old style name
    if ($node =~ /tb(pc.*)/) {
	($node = $1) =~ s/pc0/pc/;
    } elsif ($node =~ /tb(sh\d+)/) {
	($node = $1) =~ s/$/-1/;
    }

    return ($node, $alloc);
}

sub parsebatch($$$) {
    my ($str, $refstamp, $bstate) = @_;
    my $pat;
    my $stamp = 0;

    $pat = q(Beginning pre run for.* (\w+ \w+ [ \d]\d [ \d]\d:\d\d:\d\d \d\d\d\d));
    if ($str =~ /$pat/) {
	$stamp = parsedate($1, DATE_CTIME);
	return BATCHPRELOAD, $stamp;
    }
    $pat = q(Beginning pre run for.*\. ([ \d]\d:\d\d:\d\d));
    if ($str =~ /$pat/) {
	$stamp = parsetime($1, $refstamp);
	return BATCHPRELOAD, $stamp;
    }

    $pat = q(^Run finished - (\w+ \w+ [ \d]\d [ \d]\d:\d\d:\d\d \d\d\d\d));
    if ($str =~ /$pat/) {
	$stamp = parsedate($1, DATE_CTIME);
	return BATCHCREATE, $stamp;
    }
    $pat = q(Swap in finished - (\w+ \w+ [ \d]\d [ \d]\d:\d\d:\d\d \d\d\d\d));
    if ($str =~ /$pat/) {
	$stamp = parsedate($1, DATE_CTIME);
	return BATCHCREATE, $stamp;
    }
    $pat = q(Swapin finished\. ([ \d]\d:\d\d:\d\d));
    if ($str =~ /$pat/) {
	$stamp = parsetime($1, $refstamp);
	return BATCHCREATE, $stamp;
    }
    $pat = q(Successfully finished swap-in for .*\. ([ \d]\d:\d\d:\d\d)(:\d+)?$);
    if ($str =~ /$pat/) {
	$stamp = parsetime($1, $refstamp);
	return $bstate == STATE_BATCHPRELOAD ? BATCHCREATE : BATCHSWAPIN, $stamp;
    }

    #
    # See 2002.mail: <200205131730.g4DHUef19744@boss.emulab.net>
    # scale2-600 has "Swapin." followed by TIMESTAMP line.
    # just matching this TIMESTAMP line threw 2001 stuff off.
    #
    $pat = q(Swapin finished\.$);
    if ($str =~ /$pat/) {
	return BATCHCREATEMAYBE, $refstamp;
    }
    $pat = q(TIMESTAMP: ([ \d]\d:\d\d:\d\d) tbswapin finished);
    if ($str =~ /$pat/ && $bstate == STATE_BATCHCREATEMAYBE) {
	$stamp = parsetime($1, $refstamp);
	return BATCHCREATE, $stamp;
    }

    #
    # The pain never stops...  For a while, these messages had a time stamp
    # that could look like '0:NN:NN' instead of '00:NN:NN'
    #
    $pat = q(^Cleanup finished - (\w+ \w+ [ \d]\d [ \d]\d:\d\d:\d\d \d\d\d\d));
    if ($str =~ /$pat/) {
	$stamp = parsedate($1, DATE_CTIME);
	return BATCHTERM, $stamp;
    }
    $pat = q(Swap out finished - (\w+ \w+ [ \d]\d [ \d]\d:\d\d:\d\d \d\d\d\d));
    if ($str =~ /$pat/) {
	$stamp = parsedate($1, DATE_CTIME);
	return BATCHTERM, $stamp;
    }
    $pat = q(Swapout finished\. ([ \d]\d:\d\d:\d\d));
    if ($str =~ /$pat/) {
	$stamp = parsetime($1, $refstamp);
	return BATCHTERM, $stamp;
    }
    $pat = q(Successfully finished swap-out for .*\. ([ \d]\d:\d\d:\d\d):\d+$);
    if ($str =~ /$pat/) {
	$stamp = parsetime($1, $refstamp);
	return BATCHSWAPOUT, $stamp;
    }

    #
    # Message starting with 'Batch Mode experiment .* has finished':
    # pre-2002ish: this may really be a terminate message, or it may contain
    #    no useful info     
    # 2002ish: may be a combo create/destroy message.
    #
    $pat = q(Batch Mode experiment .* has finished!);
    if ($str =~ /$pat/) {
	return BATCHTERMMAYBE, $refstamp;
    }

    $pat = q(Your Batch Mode experiment has been canceled!);
    if ($str =~ /$pat/) {
	return BATCHTERMMAYBE, $refstamp;
    }

    $pat = q(You will be notified via email when the experiment has been torn down);
    if ($str =~ /$pat/) {
	return BATCHIGNORE, $refstamp;
    }

    return BAD, $stamp;
}
