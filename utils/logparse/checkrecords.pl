#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
# All rights reserved.
#

#
# Check a list of Emulab records, optionally adding fixup records
# to take care of inconsistencies.
#

use Getopt::Std;
use tbmail;

sub usage()
{
    print STDERR
          "Usage: checkrecords [-SWdfh] < recordfile [ > newrecordfile ]\n".
	  "  Check the consistency of experiment records and report.\n".
	  "  If -f is specified, perform heuristic fixups and print new,\n".
	  "  sorted record list on stdout.\n".
	  "Options:\n".
          "  -S      don't sort input records before processing\n".
          "  -W      don't whine about inconsistencies\n".
          "  -d      print (lots of!) debug info\n".
          "  -f      generate records fixing up inconsistencies\n".
	  "  -h      print this help message\n";
    exit(1);
}
my $optlist = "SWdfh";

my $fixup = 0;
my $debug = 0;
my $whine = 1;
my $sortem = 1;

my @records = ();
my $currecix = 0;
my %experiments = ();
my %nodes = ();

sub checkexpstate();
sub checkcreate($);

#
# Parse command arguments.
#
%options = ();
if (! getopts($optlist, \%options)) {
    usage();
}
if (defined($options{"S"})) {
    $sortem = 0;
}
if (defined($options{"W"})) {
    $whine = 0;
}
if (defined($options{"d"})) {
    $debug++;
}
if (defined($options{"f"})) {
    $fixup = 1;
}
if (defined($options{"h"})) {
    usage();
}

print STDERR "Reading records...\n"
    if ($debug);
my $bad = 0;
my $lineno = 1;
while (my $line = <STDIN>) {
    my $rec = parserecord($line);
    if ($rec) {
	push(@records, $rec);
    } else {
	print STDERR "*** Bad record on line $lineno:\n";
	print STDERR "    '$line'\n";
	$bad++;
    }
    $lineno++;
}

# Sort input records if desired
if ($sortem) {
    print STDERR "Sorting ", scalar(@records), " records...\n"
	if ($debug);
    @records = sortrecords(@records);
}

#
# Check consistency of experiment state w.r.t. all records
# creating fixup records if desired.
#
print STDERR "Checking consistency of ", scalar(@records), " records...\n"
    if ($debug);
checkexpstate();

#
# Merge in any fixup records, weed out any dead records (stamp <= 0)
# and print the resulting records
#
if ($fixup) {
    print STDERR "Merging ", scalar(@fixups), " fixups...\n"
	if ($debug);
    @records = sortrecords(@records, @fixups);
    if (@records > 0) {
	while ($records[0][REC_STAMP] <= 0) {
	    shift @records;
	}
    }
    print STDERR "Printing ", scalar(@records), " records...\n"
	if ($debug);
    for my $rec (@records) {
	printrecord($rec, 1);
    }
}

#
# And the stats
#
print STDERR $lineno - 1, " records processed";
print STDERR ", $bad bad records ignored"
    if ($bad);
print STDERR ", ", scalar(@fixups), " fixup records created"
    if (@fixups > 0);
print STDERR "\n";

sub getnodelist($) {
    my ($exp) = @_;

    my @nlist = ();
    foreach $node (keys %nodes) {
	next if (!$nodes{$node});
	my ($omsgid, $oexp) = @{$nodes{$node}};
	push(@nlist, $node)
	    if ($oexp eq $exp);
    }
    return @nlist;
}

#
# Verify that experiment state is consistant.
# We optionally try to fix up inconsisancies.  The fixup code can be
# mind-bendingly complex in places.
#
sub checkexpstate() {
    $currecix = 0;
    for my $rec (@records) {
	my $action = $$rec[REC_ACTION];

	# Experiment creation
	if (ISCREATE($action) || $action == BATCHCREATE || $action == PRELOAD) {
	    checkcreate($rec);
	}
	# Experiment swapout
	elsif ($action == SWAPOUT || $action == BATCHSWAPOUT) {
	    checkswapout($rec);
	}
	# Experiment swapin
	elsif ($action == SWAPIN || $action == BATCHSWAPIN) {
	    checkswapin($rec);
	}
	# Experiment termination
	elsif ($action == TERMINATE || $action == BATCHTERM) {
	    checkterminate($rec);
	}
	# Experiment modify
	elsif ($action == MODIFY) {
	    checkmodify($rec);
	}
	$currecix++;
    }
}

#
# Mark the set of nodes in the record as belonging to the experiment in
# the record.  Perform consistency checks to ensure the nodes are not
# allocated to someone else.  Generate fixups if desired.
# Called for experiment create, swapin and modify.
#
sub checkallocnodes($) {
    my $rec = shift;
    my ($stamp, $pid, $eid, $uid, $action, $msgid, @enodes) = @$rec;
    my $exp = "$pid/$eid";

    print STDERR "checkallocnodes: [$stamp $pid $eid $uid $action $msgid ",
        join(" ", @enodes), "]\n"
	if ($debug > 1);

    foreach my $node (@enodes) {
	#
	# Error: node already belongs to someone.
	# Whine, and clean out the old association.
	#
	if ($nodes{$node}) {
	    my ($n_msgid, $n_exp) = @{$nodes{$node}};
	    if ($whine) {
		print STDERR "*** Node $node already allocated to '$n_exp'\n".
		    "    This alloc: $msgid\n".
		    "    Last alloc: $n_msgid\n"
		}

	    #
	    # Fixup: if the other experiment still exists, we are probably
	    # missing the SWAPOUT/TERMINATE information.  So we generate a
	    # SWAPOUT record for the experiment that the node belongs to.
	    # We record the index of this fake record, in case we later
	    # discover that this should have been a TERMINATE (ie., we later
	    # try to CREATE an experiment with the same name).
	    #
	    # XXX if this is an old-style create record, just fake up a
	    # TERMINATE as there was no SWAPOUT then.
	    #
	    if ($experiments{$n_exp}) {
		my @e_nlist = getnodelist($n_exp);
		my ($n_pid, $n_eid) = split("/", $n_exp);
		my $nrec;
		($e_uid, undef, $e_state, undef) = @{$experiments{$n_exp}};
		if ($fixup) {
		    if ($e_state eq "IN") {
			$nrec = [$stamp-1, $n_pid, $n_eid, $e_uid,
				 $action == CREATE1 ? TERMINATE :
				 ($action == BATCHCREATE ? BATCHSWAPOUT :
				  SWAPOUT), "<FAKE>", @e_nlist];
			push(@fixups, $nrec);
		    }
		}
		#
		# Since we marked the old experiment as swapped,
		# fixup its state, removing any other nodes allocated
		# to it and marking it as swapped/terminated.
		#
		map { undef $nodes{$_} } @e_nlist;
		if ($action == CREATE1) {
		    undef $experiments{$n_exp};
		} else {
		    $experiments{$n_exp} = [ $uid, $msgid, "OUT", $nrec ];
		}
	    }
	    undef $nodes{$node};
	}
	$nodes{$node} = [ $msgid, $exp ];
    }
}

#
# Mark the set of nodes in the record as being free.
# Perform consistency checks to ensure the nodes are actually allocated
# and allocated to the correct experiment and not someone else.
# Also ensure that no other nodes are marked as allocated to the experiment.
# Generate fixups if desired.  Called for experiment swapout and terminate.
#
sub checkfreenodes($) {
    my $rec = shift;
    my ($stamp, $pid, $eid, $uid, $action, $msgid, @enodes) = @$rec;
    my $exp = "$pid/$eid";

    print STDERR "checkfreenodes: [$stamp $pid $eid $uid $action $msgid ",
                 join(" ", @enodes), "]\n"
	if ($debug > 1);

    #
    # Fixup: it is possible that the swapout/terminate record doesn't contain
    # a node list or that the node list doesn't match what we have in our
    # record.  Since we cannot go back and fix the allocation record, we
    # will just tweak-out our node list to match.  So as we loop through the
    # nodes, we build up a new list to match the allocation state.
    #
    my @nlist = ();
    my $needfixin = 0;

    #
    # Common case: terminate/swap records that don't include a node list
    # at all.  Don't complain about these.
    #
    my $dowhine = $whine;
    $dowhine = 0
	if (@enodes == 0);

    foreach my $node (@enodes) {
	# Node is marked as allocated in the table
	if ($nodes{$node}) {
	    my ($n_msgid, $n_exp) = @{$nodes{$node}};

	    #
	    # Error: node not allocated to us.  Probably a missing
	    # SWAPOUT/TERMINATE record for the other experiment.
	    # Do as we do in allocfreenodes() above.
	    #
	    if ($n_exp ne $exp) {
		print STDERR "*** Deallocating node $node belonging to '$n_exp'\n".
		             "    This exp:  $msgid\n".
		             "    Other exp: $n_msgid\n"
			     if ($dowhine);

		my @e_nlist = getnodelist($n_exp);
		my ($n_pid, $n_eid) = split("/", $n_exp);
		my $nrec;
		($e_uid, undef, $e_state, undef) = @{$experiments{$n_exp}};
		if ($fixup) {
		    if ($e_state eq "IN") {
			$nrec = [$stamp-1, $n_pid, $n_eid, $e_uid,
				 $action == CREATE1 ? TERMINATE :
				 ($action == BATCHCREATE ? BATCHSWAPOUT :
				  SWAPOUT), "<FAKE>", @e_nlist];
			push(@fixups, $nrec);
		    }
		}
		#
		# Since we marked the old experiment as swapped,
		# fixup its state, removing any other nodes allocated
		# to it and marking it as swapped.
		#
		map { undef $nodes{$_} } @e_nlist;
		$experiments{$n_exp} = [ $e_uid, $msgid, "OUT", $nrec ];
	    } else {
		# It is ours, so free it
		undef $nodes{$node};
	    }
	    # remember that the node belongs to us for later fixup
	    push(@nlist, $node)
		if ($fixup);
	}
	#
	# Error: node marked as unallocated.
	# Add to our node list so we can fix up this record.
	#
	else {
	    print STDERR "*** Attempt to deallocate free node $node\n".
		"    Msg: $msgid\n"
		if ($dowhine);
	    $needfixin = $fixup;
	}
    }

    #
    # Run through node list to see if there are other nodes that
    # belong to us that were not listed.  If so, free it and add it
    # to our fixup list.
    #
    foreach $node (keys %nodes) {
	next if (!$nodes{$node});
	my ($n_msgid, $n_exp) = @{$nodes{$node}};
	if ($n_exp eq $exp) {
	    print STDERR "*** Did not free node $node allocated to '$exp'\n".
		"    Msg: $msgid\n".
		"    Allocation: $n_msgid\n"
		if ($dowhine);
	    undef $nodes{$node};

	    $needfixin = $fixup;
	    push(@nlist, $node)
		if ($fixup);
	}
    }

    #
    # Fixup: message node list did not match what we had in our state.
    # Build a new record with the correct list, and mark the original record
    # as "DEAD" (stamp == 0).
    #
    if ($needfixin) {
	push(@fixups,
	     [$stamp, $pid, $eid, $uid, $action, $msgid, @nlist]);
	$$rec[REC_STAMP] = 0;
    }
}


#
# Create an experiment
#
sub checkcreate($) {
    my $rec = shift;
    my ($stamp, $pid, $eid, $uid, $action, $msgid, @enodes) = @$rec;
    my $exp = "$pid/$eid";

    print STDERR "checkcreate: [$stamp $pid $eid $uid $action $msgid ",
                 join(" ", @enodes), "]\n"
	if ($debug > 1);

    #
    # Error: experiment already exists
    #
    if ($experiments{$exp}) {
	($e_uid, $e_msgid, $e_state, $e_rec) = @{$experiments{$exp}};
	my @e_nlist = getnodelist($exp);

	#
	# Fixup: first see if this was a problem we caused by faking up an
	# earlier SWAPOUT record when we should have done a TERMINATE.
	# If so patch up that record to be a TERMINATE.  We also check to
	# see if we are a duplicate BATCHCREATE message and invalidate the
	# first record if so.  If neither of these cases hold, we need to
	# generate a TERMINATE record.
	#
	if ($fixup) {
	    if (defined($e_rec)) {
		if ($e_rec->[REC_ACTION] eq SWAPOUT) {
		    $e_rec->[REC_ACTION] = TERMINATE;
		} elsif ($e_rec->[REC_ACTION] eq BATCHSWAPOUT) {
		    $e_rec->[REC_ACTION] = BATCHTERM;
		} elsif ($e_rec->[REC_ACTION] eq BATCHCREATE) {
		    if ($e_rec->[REC_ACTION] eq $action &&
			$e_rec->[REC_STAMP] == $stamp &&
			$e_rec->[REC_UID] eq $uid &&
			# XXX should compare actual list contents
			scalar(@e_nlist) == scalar(@enodes)) {
			$e_rec->[REC_STAMP] = 0;
			print STDERR "*** Eliminate redundent BATCHCREATE".
			             " for experiment '$exp'\n".
				     "    This creation: $msgid\n".
				     "    Last creation: $e_msgid\n"
				     if ($whine);
		    } else {
			push(@fixups,
			     [$stamp-1, $pid, $eid, $e_uid, BATCHTERM, "<FAKE>",
			      @e_nlist]);
		    }
		}
	    } else {
		push(@fixups,
		     [$stamp-1, $pid, $eid, $e_uid, TERMINATE, "<FAKE>",
		      @e_nlist]);
	    }
	}

	#
	# Otherwise just whine about it
	#
	elsif ($whine) {
	    print STDERR "*** Attempt to ", ACTIONSTR($action),
	                 " existing experiment '$exp'\n".
			 "    This creation: $msgid\n".
			 "    Last creation: $e_msgid\n";
	}

	#
	# Cleanup any internal state associated with the old instance so we
	# can fill in ours.
	#
	map { undef $nodes{$_} } @e_nlist;
	undef $experiments{$exp};
    }

    #
    # Check the list of nodes in the record, ensuring that they are not
    # already allocated and assigning them to us.
    #
    if (@enodes > 0) {
	if ($action != PRELOAD) {
	    checkallocnodes($rec);
	} else {
	    print STDERR "*** PRELOAD of '$exp' involves nodes\n".
		         "    Msgid: $msgid\n"
			 if ($whine);
	}
    }

    #
    # Experiment now exists
    #
    # For BATCHCREATE, we remember this record's index.  It is possible
    # that a redundent CREATE record was generated circa 2001 when we
    # send out both a "batch started" message with the startup info
    # and a "batch done" message with both the start and end info.
    #
    $experiments{$exp} = [ $uid, $msgid, $action == PRELOAD ? "OUT" : "IN",
			   $action == BATCHCREATE ? $rec : undef ];
}

sub checkswapout($) {
    my $rec = shift;
    my ($stamp, $pid, $eid, $uid, $action, $msgid, @enodes) = @$rec;
    my $exp = "$pid/$eid";

    print STDERR "checkswapout: [$stamp $pid $eid $uid $action $msgid ",
                 join(" ", @enodes), "]\n"
	if ($debug > 1);

    #
    # Error: experiment does not exist
    # Whine if necessary, create the experiment and generate a fixup.
    # Just use checkcreate() to make everything is correct.
    #
    if (!$experiments{$exp}) {
	print STDERR "*** Attempt to ", ACTIONSTR($action),
	             " nonexistent experiment '$exp'\n".
	             "    Msg: $msgid\n"
		     if ($whine);

	my $rec = [$stamp-1, $pid, $eid, $uid, CREATE2, "<FAKE>", @enodes];
	checkcreate($rec);

	#
	# Fixup: record the fake CREATE record.
	#
	if ($fixup) {
	    push(@fixups, $rec);
	}
    }

    (undef, $e_msgid, $e_state, undef) = @{$experiments{$exp}};

    #
    # Error: experiment exists but was not swapped in
    # Whine if necessary, swapin the experiment and generate a fixup.
    # Just use checkswapin() to make everything is correct.
    #
    if ($e_state ne "IN") {
	print STDERR "*** Attempt to ", ACTIONSTR($action),
	             " swapped experiment '$exp'\n".
	             "    Msg: $msgid\n"
		     if ($whine);

	my $rec = [$stamp-1, $pid, $eid, $uid, SWAPIN, "<FAKE>", @enodes];
	checkswapin($rec);

	#
	# Fixup: record the fake SWAPIN record.
	#
	if ($fixup) {
	    push(@fixups, $rec);
	}
    }

    #
    # Check the list of nodes in the record, ensuring that they are
    # allocated to us.
    #
    checkfreenodes($rec);

    # Mark experiment as swapped
    $experiments{$exp} = [ $uid, $msgid, "OUT", undef ];
}

#
# Swapin an experiment
#
sub checkswapin($) {
    my $rec = shift;
    my ($stamp, $pid, $eid, $uid, $action, $msgid, @enodes) = @$rec;
    my $exp = "$pid/$eid";

    print STDERR "checkswapin: [$stamp $pid $eid $uid $action $msgid ",
                 join(" ", @enodes), "]\n"
	if ($debug > 1);

    #
    # Error: experiment does not exist.
    # Create it as swapped out.
    #
    if (!$experiments{$exp}) {
	print STDERR "*** Attempt to ", ACTIONSTR($action),
	             " nonexistent experiment '$exp'\n".
	             "    Msg: $msgid\n"
		     if ($whine);

	#
	# Fixup: generate a PRELOAD record
	#
	if ($fixup) {
	    push(@fixups,
		 [$stamp-1, $pid, $eid, $uid, PRELOAD, "<FAKE>"]);
	}
	$experiments{$exp} = [ $uid, "FAKE", "OUT", undef ];
    }

    ($e_uid, $e_msgid, $e_state, undef) = @{$experiments{$exp}};

    #
    # Error: already swapped in
    #
    if ($e_state ne "OUT") {
	print STDERR "*** Attempt to ", ACTIONSTR($action),
	             " swapped in experiment '$exp'\n".
		     "    Msg: $msgid\n"
		     if ($whine);
	
	my @e_nlist = getnodelist($exp);
	my $nrec;
	
	#
	# Fixup: fake a swapout with the existing set of nodes
	#
	if ($fixup) {
	    $nrec = [$stamp-1, $pid, $eid, $e_uid,
		     $action == SWAPIN ? SWAPOUT : BATCHSWAPOUT,
		     "<FAKE>", @e_nlist];
	    push(@fixups, $nrec);
	}
	#
	# Since we marked the old experiment as swapped,
	# fixup its state, removing any other nodes allocated
	# to it and marking it as swapped.
	#
	map { undef $nodes{$_} } @e_nlist;
	$experiments{$exp} = [ $uid, $msgid, "OUT", $nrec ];
    }

    #
    # We are swapped out, just check the node list
    #
    checkallocnodes($rec);

    # Experiment is now swapped in
    $experiments{$exp} = [ $uid, $msgid, "IN", undef ];
}

#
# Destroy an experiment
#
sub checkterminate($) {
    my $rec = shift;
    my ($stamp, $pid, $eid, $uid, $action, $msgid, @enodes) = @$rec;
    my $exp = "$pid/$eid";

    print STDERR "checkterminate: [$stamp $pid $eid $uid $action $msgid ",
                 join(" ", @enodes), "]\n"
	if ($debug > 1);

    # Error: experiment was never created
    if (!$experiments{$exp}) {
	print STDERR "*** Attempt to ", ACTIONSTR($action),
	             " nonexistent experiment '$exp'\n".
		     "    Msg: $msgid\n"
		     if ($whine);

	my $nrec = [$stamp-1, $pid, $eid, $uid,
		    $action == TERMINATE ? CREATE2 : BATCHCREATE,
		    "<FAKE>", @enodes];
	checkcreate($nrec);

	#
	# Fixup: record the fake CREATE record.
	#
	if ($fixup) {
	    push(@fixups, $nrec);
	}
    }

    checkfreenodes($rec);

    # Experiment is gone
    undef $experiments{$exp};
}

#
# Modify an experiment
#
sub checkmodify($) {
    my $rec = shift;
    my ($stamp, $pid, $eid, $uid, $action, $msgid, @enodes) = @$rec;
    my $exp = "$pid/$eid";

    print STDERR "checkmodify: [$stamp $pid $eid $uid $action $msgid ",
                 join(" ", @enodes), "]\n"
	if ($debug > 1);

    #
    # Error: experiment doesn't exist.
    # Whine if necessary, and create the experiment empty.
    #
    if (!$experiments{$exp}) {
	print STDERR "*** Attempt to modify nonexistent experiment '$exp'\n".
	             "    Msg: $msgid\n"
		     if ($whine);

	#
	# Fixup: record the fake CREATE record.
	#
	if ($fixup) {
	    push(@fixups, [$stamp-1, $pid, $eid, $uid, CREATE2, "<FAKE>"]);
	}

	# Experiment now exists
	$experiments{$exp} = [ $uid, $msgid, "IN", undef ];
    }

    (undef, $e_msgid, $e_state, undef) = @{$experiments{$exp}};

    #
    # Error: modify has nodes but experiment is swapped out.
    # Whine and fake up an empty swapin if desired.
    #
    if (@enodes > 0 && $e_state ne "IN") {
	print STDERR "*** Attempt to modify swapped experiment '$exp'\n".
	             "    Msg:  $msgid\n".
	             "    Swap: $e_msgid\n"
		     if ($whine);

	#
	# Fixup: record the fake SWAPIN record.
	#
	if ($fixup) {
	    push(@fixups, [$stamp-1, $pid, $eid, $uid, SWAPIN, "<FAKE>"]);
	}

	# Experiment is now swapped in
	$experiments{$exp} = [ $uid, $msgid, "IN", undef ];
    }

    #
    # Modify records contain the list of nodes in the experiment after
    # the modify completes.  This may involve adding new nodes and removing
    # old nodes.  Right now we do this by just removing everything listed
    # as belonging to us, and then readding all the nodes in the list
    # (which could generate fixups).
    #
    my @nlist = getnodelist($exp);

    #
    # XXX possibly temporary
    # Generate a list of added/removed nodes and add fake records for them.
    #
    if ($fixup) {
	my %nhash = ();
	map { $nhash{$_} = 1 } @nlist;
	my @delta = ();
	for my $node (@enodes) {
	    if ($nhash{$node}) {
		undef $nhash{$node};
	    } else {
		push(@delta, $node);
	    }
	}
	push(@fixups,
	     [$stamp, $pid, $eid, $uid, MODIFYADD, "<FAKE>", @delta])
	    if (@delta > 0);
	@delta = grep { $nhash{$_} } keys(%nhash);
	push(@fixups,
	     [$stamp, $pid, $eid, $uid, MODIFYSUB, "<FAKE>", @delta])
	    if (@delta > 0);
    }

    map { undef $nodes{$_} } @nlist;
    checkallocnodes($rec);
}

