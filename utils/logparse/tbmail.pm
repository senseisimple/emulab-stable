#!/usr/bin/perl -wT
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2004, 2006 University of Utah and the Flux Group.
# All rights reserved.
#

package tbmail;
use Exporter;
@ISA    = "Exporter";
@EXPORT = qw(makerecord parserecord printrecord sortrecords
	     REC_STAMP REC_PID REC_EID REC_UID REC_ACTION REC_MSGID REC_NODES
	     BAD IGNORE PRELOAD MODIFY MODIFYADD MODIFYSUB CREATE1 CREATE2
	     ISCREATE SWAPIN SWAPOUT SWAPOUTMAYBE TERMINATE BATCH BATCHIGNORE
	     BATCHCREATE BATCHCREATEMAYBE BATCHTERM BATCHTERMMAYBE BATCHSWAPIN
	     BATCHSWAPOUT BATCHPRELOAD ACTIONSTR
);

# Must come after package declaration!
use English;

# Load up the paths. Done like this in case init code is needed.
BEGIN
{
}

#
# Experiment records
# Record format is:
#	timestamp pid eid uid action msgid nodelist
#
sub REC_STAMP	{ return 0 };
sub REC_PID	{ return 1 };
sub REC_EID	{ return 2 };
sub REC_UID	{ return 3 };
sub REC_ACTION	{ return 4 };
sub REC_MSGID	{ return 5 };
sub REC_NODES	{ return 6 };

#
# Actions
#
sub BAD()	{ return 0 };
sub IGNORE()	{ return 1 };

sub PRELOAD()	{ return 2 };
sub MODIFY()	{ return 3 };
sub MODIFYADD()	{ return 4 };
sub MODIFYSUB()	{ return 5 };

sub CREATE1()	{ return 11 };	# 2000-ish creation message
sub CREATE2()	{ return 12 };	# 2001-ish creation message
sub ISCREATE($)
    { my $id = shift; return ($id && $id >= CREATE1 && $id <= CREATE2); }

sub SWAPIN()	    { return 21 };
sub SWAPOUT()	    { return 31 };
sub SWAPOUTMAYBE()  { return 32 };

sub TERMINATE()	{ return 41 };

sub BATCH()	       { return 50 };
sub BATCHIGNORE()      { return 51 };
sub BATCHCREATE()      { return 52 };
sub BATCHCREATEMAYBE() { return 53 };
sub BATCHTERM()	       { return 54 };
sub BATCHTERMMAYBE()   { return 55 };
sub BATCHSWAPIN()      { return 56 };
sub BATCHSWAPOUT()     { return 57 };
sub BATCHPRELOAD()     { return 58 };

my %actiontostr = (
    BAD()          => 'BAD',
    IGNORE()       => 'IGNORE',
    PRELOAD()      => 'PRELOAD',
    MODIFY()       => 'MODIFY',
    MODIFYADD()    => 'MODIFYADD',
    MODIFYSUB()    => 'MODIFYSUB',
    CREATE1()      => 'OCREATE',
    CREATE2()      => 'CREATE',
    SWAPIN()       => 'SWAPIN',
    SWAPOUT()      => 'SWAPOUT',
    TERMINATE()    => 'TERMINATE',
    BATCH()        => 'BATCH',
    BATCHCREATE()  => 'BATCHCREATE',
    BATCHTERM()    => 'BATCHTERM',
    BATCHSWAPIN()  => 'BATCHSWAPIN',
    BATCHSWAPOUT() => 'BATCHSWAPOUT',
);

my %strtoaction = (
    'BAD'         => BAD(),
    'IGNORE'      => IGNORE(),
    'PRELOAD'     => PRELOAD(),
    'MODIFY'      => MODIFY(),
    'MODIFYADD'   => MODIFYADD(),
    'MODIFYSUB'   => MODIFYSUB(),
    'OCREATE'     => CREATE1(),
    'CREATE'      => CREATE2(),
    'SWAPIN'      => SWAPIN(),
    'SWAPOUT'     => SWAPOUT(),
    'TERMINATE'   => TERMINATE(),
    'BATCH'       => BATCH(),
    'BATCHCREATE' => BATCHCREATE(),
    'BATCHTERM'   => BATCHTERM(),
    'BATCHSWAPIN' => BATCHSWAPIN(),
    'BATCHSWAPOUT'=> BATCHSWAPOUT(),
);

sub ACTIONSTR($) { my $a = shift; return $actiontostr{$a} };

#
# Make sure we create/consume records in a consistent way
#
sub makerecord($$$$$$@) {
    my ($stamp, $pid, $eid, $uid, $action, $msgid, @nodes) = @_;
    return [ $stamp, $pid, $eid, $uid, $action, $msgid, @nodes ];
}

sub parserecord($) {
    my $l = shift;
    my ($stamp, $pid, $eid, $uid, $action, $msgid, @nodes) = split(/\s+/, $l);
    $action = $strtoaction{$action};
    return undef if (!$action);
    return [ $stamp, $pid, $eid, $uid, $action, $msgid, @nodes ];
}

#
# Sort records: first by timestamp, then by pid/eid, uid, real/fake
#
sub recsort {
    return $a->[REC_STAMP] <=> $b->[REC_STAMP] ||
	$a->[REC_PID] cmp $b->[REC_PID] ||
	$a->[REC_EID] cmp $b->[REC_EID] ||
	$a->[REC_UID] cmp $b->[REC_UID] ||
	$a->[REC_MSGID] cmp $b->[REC_MSGID];
}
sub sortrecords(@) {
    return sort recsort @_;
}

#
# Sort nodes: pcs before sharks, then in numeric order
#
sub nodesort {
    # sort by name prefix
    my $as = $1 if ($a =~ /^(\D+)/);
    my $bs = $1 if ($b =~ /^(\D+)/);
    if (my $sc = $as cmp $bs) {
	return $sc;
    }

    # shark hack, take out '-'
    (my $an = $a) =~ s/-//;
    (my $bn = $b) =~ s/-//;

    # sort by number
    $an = $1 if ($an =~ /(\d+)$/);
    $bn = $1 if ($bn =~ /(\d+)$/);
    return $an <=> $bn if ($an && $bn);
    return $a cmp $b;
}

sub printrecord($$) {
    my ($rec, $sortem) = @_;
    my ($stamp, $pid, $eid, $uid, $action, $msgid, @nodes) = @{$rec};

    my $actstr = $actiontostr{$action};
    if ($sortem && @nodes > 1) {
	@nodes = sort nodesort @nodes;
    }

    print "$stamp $pid $eid $uid $actstr $msgid";
    if (@nodes > 0) {
	print " ", join(" ", @nodes);
    }
    print "\n";
}
