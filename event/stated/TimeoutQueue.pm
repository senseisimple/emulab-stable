#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2003-2003 University of Utah and the Flux Group.
# All rights reserved.
#

#
# timeout-queue.pm
#
# An implementation of a Priority Queue, customized for dealing with
# timeouts. It maintains a queue, prioritized by timeout, with FIFO
# behavior for things with the same timeout.  You insert items along
# with their timeout value, and they replace any other timeouts for
# that object. You can also examine the head of the queue, pop from
# the head of the queue, or delete an item from the queue. You can
# also ask for a given object, when its next timeout occurs.
#
# The API:
# qinsert($timeout,$obj) - returns 0
#   Insert an object, or update its timeout
# qdelete($obj)		 - returns 0, or 1 if not found
#   Delete an object
# qhead(\$timeout,\$obj) - returns 0, or 1 if not found
#   Look at the head item
# qpop(\$timeout,\$obj)	 - returns 0, or 1 if empty
#   Remove and return the head item
# qfind($obj)		 - returns timeout, or undef if not found
#   Find the timeout for an item
# qsize([$timeout])	 - returns queue size
#   Return the number of items in the queue
# qshow([$timeout])	 - returns 0
#   Print out the contents of the queue, or for a given timeout
#

# You could also use this for anything that liked to have things sorted
# with the lowest key at the head of the queue. (It could also be changed
# to allow the highest at the head, but I'm not doing it until I need it.)
#
# Related work: Heap::Priority and List::Priority on CPAN
# (They don't support some of the lookup and removal options that a
# timeout queue needs.)
#

#
# This is intentially designed not to depend on being used within Emulab
# software. Any functions that this library depends on should be documented
# here:
#
#

package TimeoutQueue;

use Exporter;
@ISA = "Exporter";
@EXPORT =
    qw ( qinsert qdelete qhead qpop qfind qsize qshow);

# Any 'use' or 'require' stuff must come here, after 'package'

#
# Package variables
#
$debug = 0;
@q = (); # The queue
%i = (); # The index

#
# Internals:
#
# The queue is kept as a sorted array of pairs (references of two item
# arrays). (The alternative is to keep sorting a hash everytime we
# need to do anything with the head.)  To make finding an item easy,
# we also keep a hash with a mapping of obj->timeout.
#

#
# Exported Package Subroutines/Functions
#

# qinsert($timeout,$obj) - returns 0
#   Insert an object, or update its timeout
sub qinsert {
    my ($timeout, $obj) = @_;
    debug("Inserting $timeout -> $obj\n");
    if (defined($i{$obj})) {
	# Already in there... take it out
	qdelete($obj);
    }
    my $loc = qsearch($timeout,0);
    my @l = ($timeout,$obj);
    debug("Splicing in ($timeout -> $obj) [".\@l."] at $loc\n");
    splice(@q,$loc,0,\@l);
    if ($debug>1) { qshow(); }
    $i{$obj} = $timeout;
    return 0;
}

# qdelete($obj)		 - returns 0, or 1 if not found
#   Delete an object
sub qdelete {
    my ($obj) = @_;
    if (!defined($i{$obj})) {
	return 1;
    }
    my $timeout = $i{$obj};
    my $n=qsearch($timeout,1);
    debug("Delete $obj at $n\n");
    my $end = @q+0;
    while (1) {
	$o = ${$q[$n]}[1];
        debug("Checking $o == $obj at $n\n");
	if ($o eq $obj) {
	    debug("Splicing out $n\n");
	    splice(@q,$n,1);
	    if ($debug>1) { qshow(); }
	    last;
	}
	$n++;
	if ($n > $end) { return 1;}
    }
    delete $i{$obj};
    return 0;
}

# qhead(\$timeout,\$obj) - returns 0, or 1 if not found
#   Look at the head item
sub qhead {
    if (@q+0 == 0) { $_[0]=undef; $_[1]=undef; return 1; }
    debug("\thead=".${$q[0]}[0]."->".${$q[0]}[1]."\n");
    $_[0] = ${$q[0]}[0];
    $_[1] = ${$q[0]}[1];
    return 0;
}

# qpop(\$timeout,\$obj)	 - returns 0, or 1 if empty
#   Remove and return the head item
sub qpop {
    if (@q+0 == 0) { $_[0]=undef; $_[1]=undef; return 1; }
    debug("\tpop=".${$q[0]}[0]."->".${$q[0]}[1]."\n");
    $_[0] = ${$q[0]}[0];
    $_[1] = ${$q[0]}[1];
    shift(@q);
    return 0;
}


# qfind($obj)		 - returns timeout, or undef if not found
#   Find the timeout for an item
sub qfind {
    my ($obj) = @_;
    return $i{$obj};
}

# qsize([$timeout])	 - returns queue size
#   Return the number of items in the queue
sub qsize {
    return 0+@q;
}

# qshow([$timeout])	 - returns 0
#   Print out the contents of the queue, or for a given timeout
sub qshow {
    print "The TimeoutQueue:\n";
    if (@_ > 0) {
	my ($timeout) = @_;
	# print just one level
	print "Not implemented for single level ($timeout)...\n";
    } else {
	# Print all of it
	foreach $p (@q) {
	    print "  Q: ${$p}[0] -> ${$p}[1]\t[$p]\n";
	}
    }
    return 0;
}

#
# Utility Subroutines/Functions
#

# qsearch($timeout,$first) - returns index
#   Find the index in @q where ($first ? $timout starts : $timeout ends)
sub qsearch {
    my ($timeout,$first) = @_;
    # locally disable debug output
    local($debug) = 0;
    debug("Searching: $timeout (first=$first)\n");
    return qbinsearch($timeout,0,@q+0,$first);
}

# qbinsearch($timeout,$min,$max,$first) - returns index
#   Find the index in @q where ($first ? $timout starts : $timeout ends)
sub qbinsearch {
    my ($timeout,$min,$max,$first) = @_;
    # locally disable debug output
    #local($debug) = 0;
    # Implement a binary search
    my $len = $max - $min;
    my $mid = $min + int($len/2);
    if ($len < 1) { return $mid; }
    my $val = ${$q[$mid]}[0];
    debug("\tt=$timeout  $min-$max ($len) mid $mid -> $val\n");
    if ($first) {
	if ($val >= $timeout) { return qbinsearch($timeout,$min,$mid,$first); }
	else { return qbinsearch($timeout,$mid+1,$max,$first); }
    } else {
	if ($val > $timeout) { return qbinsearch($timeout,$min,$mid,$first); }
	else { return qbinsearch($timeout,$mid+1,$max,$first); }
    }
}

sub debug {
    if ($debug) { print @_; }
}

if (0) {
    # Enable this section to do some self tests...
    local($debug);
    my ($t,$o);
    $debug=1;
    qshow();
    qinsert("10","foo");
    qshow();
    qinsert("20","bar");
    qinsert("200","baz");
    $debug=0;
    qshow();
    qinsert("10","doublefoo");
    qshow();
    qinsert("15","triplefoo");
    qshow();
    qhead($t,$o);
    print "HEAD== $t -> $o\n";
    $debug=1;
    qinsert("10","foo");
    qhead($t,$o);
    print "HEAD== $t -> $o\n";
    qshow();
    qinsert("5","foo");
    $debug=0;
    qhead($t,$o);
    print "HEAD== $t -> $o\n";
    qshow();
    qdelete("doublefoo");
    qshow();
    qdelete("triplefoo");
    qshow();
    qpop($t,$o);
    print " POP== $t -> $o\n";
    qshow();
    qpop($t,$o);
    print " POP== $t -> $o\n";
    qshow();
    qpop($t,$o);
    print " POP== $t -> $o\n";
    qshow();
    qpop($t,$o);
    print " POP== $t -> $o\n";
    qshow();
    qhead($t,$o);
    print "HEAD== $t -> $o\n";
    qshow();
}

# Always end a package successfully!
1;

