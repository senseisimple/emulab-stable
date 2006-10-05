#!/usr/bin/perl

########################
# TODO:
# 1) provide a coherency protocol to the site/node id cache
# 2) buffer updates to 1 per 5 seconds (or so)
#######################


#
# Configure variables
#

use lib '/usr/testbed/lib';
use libtbdb;
use libwanetmon;
use Getopt::Std;
use strict;
use IO::Socket::INET;
use IO::Select;

# node and site id caches
my %nodeids;
my %siteids;

# ipaddr cache, since nodes are addressed via IP in the event system.
my %ipaddrs;

# Batch up insertions. Simple string.
my $insertions = "";
my $batchsize  = 0;
my $maxbatch   = 30;
my $maxidletime= 5;	# Seconds before forced insert.
my $lastinsert = 0;	# Timestamp.

my %lasttimestamp;  #prevents adding duplicate entries in DB
my %duplicatecnt;   #counters keeping track of duplicate entries.
my $duplicateKillThresh = 50; #kill node if this many duplicates occur per index

#
# Turn off line buffering on output
#
$| = 1;

sub usage {
	print "Usage: $0 [-p receiveport] [-a sendport] [-e pid/eid]".
	    " [-d debuglevel] [-i]\n";
	return 1;
}

my $debug    = 0;
my $impotent = 0;
my $debug = 0;
my ($port, $sendport, $expid);
my %opt = ();
if (!getopts("p:a:e:d:ih", \%opt)) {
    exit &usage;
}
if ($opt{p}) { $port = $opt{p}; } else { $port = 5051; }
if ($opt{a}) { $sendport = $opt{a}; } else { $sendport = 5050; }
if ($opt{h}) { exit &usage; }
if ($opt{e}) { $expid = $opt{e}; } else { $expid = "none"; }
if ($opt{d}) { $debug = $opt{d}; } else { $debug = 0; }
if ($opt{i}) { $impotent = 1; } 

if (@ARGV !=0) { exit &usage; }

print "pid/eid = $expid\n";
print "receiveport = $port\n";
print "sendport = $sendport\n";

my $PWDFILE = "/usr/testbed/etc/pelabdb.pwd";
##TODO: CHANGE TO "pelab" and "pelab"
my $DBNAME = "pelab";
my $DBUSER = "pelab";
my $DBErrorString = "";
#TODO: change this back to file
#my $DBPWD = "";
my $DBPWD = `cat $PWDFILE`;
if( $DBPWD =~ /^([\w]*)\s([\w]*)$/ ) {
    $DBPWD = $1;
}else{
    fatal("Bad chars in password!");
}

#connect to database
my ($DB_data, $DB_sitemap);
TBDBConnect($DBNAME,$DBUSER,$DBPWD);

my $socket_rcv = IO::Socket::INET->new( LocalPort => $port,
					Proto     => 'udp' );

my $sel = IO::Select->new();
$sel->add($socket_rcv);

#
# MAIN LOOP
#

while (1) {

    #check for pending received events
#    event_poll_blocking($handle, 1000);

    SendBatchedInserts()
	if ($batchsize && (time() - $lastinsert) > $maxidletime);

    handleincomingmsgs();

}

#############################################################################

exit(0);


sub handleincomingmsgs()
{
    my $inmsg;
    #check for pending received results
    my @ready = $sel->can_read(1000);
    foreach my $handle (@ready){
	$socket_rcv->recv( $inmsg, 2048 );
	chomp $inmsg;
	print "debug: got a udp message: $inmsg\n" if( $debug > 2 );
	my %inhash = %{ deserialize_hash( $inmsg )};
#	foreach my $key (keys %inhash){
#	    print "key=$key\n";
#	    print "$key  \t$inhash{$key}\n";
#	}
	my ($exp_in, $linksrc, $linkdest, $testtype, $result, $tstamp, $index)
	    = ($inhash{expid}, $inhash{linksrc}, $inhash{linkdest},
	       $inhash{testtype}, $inhash{result}, $inhash{tstamp},
	       $inhash{index});

	# if incoming result is not of this expid, return
	if( $exp_in ne $expid ){
	    print "ignored msg from expid=$exp_in\n" if( $debug > 2 );
	    return;
	}

	print "\n" if( $debug > 1 );
	print("linksrc=$linksrc\n".
	      "linkdest=$linkdest\n".
	      "testtype =$testtype\n".
	      "result=$result\n".
	      "index=$index\n".
	      "tstamp=$tstamp\n") if( $debug > 1 );

	if( defined $linksrc ){
	    my $socket_snd = 
	      IO::Socket::INET->new( PeerPort => $sendport,
				     Proto    => 'udp',
				     PeerAddr => "$linksrc");
	    my %ack = ( expid   => $expid,
			cmdtype => "ACK",
			index   => $index,
			tstamp  => $tstamp );
	    if( defined %ack && defined $socket_snd ){
		my $ack_serial = serialize_hash( \%ack );
		$socket_snd->send($ack_serial);
		print "**SENT ACK**\n" if( $debug > 1 );

		if( !defined $lasttimestamp{$linksrc}{$index} ||
		    $tstamp ne $lasttimestamp{$linksrc}{$index} )
		{
		    saveTestToDB(linksrc   => $linksrc,
				 linkdest  => $linkdest,
				 testtype  => $testtype,
				 result    => $result,
				 tstamp    => $tstamp );
		    #clear duplicatecnt for corresponding result index
		    if( defined($duplicatecnt{$linksrc}{$index}) ){
			delete $duplicatecnt{$linksrc}{$index};
		    }
		}else{
		    print("++++++duplicate data\n".
		          "linksrc=$linksrc\n".
			  "linkdest=$linkdest\n".
			  "testtype =$testtype\n".
			  "result=$result\n".
			  "index=$index\n".
			  "tstamp=$tstamp\n") if( $debug > 0);

		    #increment duplicatecnt for this src and index number
		    if( defined($duplicatecnt{$linksrc}{$index}) ){
			$duplicatecnt{$linksrc}{$index}++;
			#kill off offending node, if > threshold
			if( $duplicatecnt{$linksrc}{$index}
			    > $duplicateKillThresh )
			{
			    killnode($linksrc);
			    print "KILLING OFF BGMON at $linksrc".
				" for index $index\n" if( $debug > 0 );
			    delete $duplicatecnt{$linksrc};
			}
		    }else{
			$duplicatecnt{$linksrc}{$index} = 1;
		    }
		    

		}
		$lasttimestamp{$linksrc}{$index} = $tstamp;
	    }

	}

    }

}


#############################################################################

sub saveTestToDB()
{
    my %results = @_;

    my @tmp;
    my ($srcsite, $srcnode, $dstsite, $dstnode);

    if( !exists $siteids{$results{linksrc}} ){
	@tmp = DBQuery("SELECT site_idx FROM site_mapping ".
		       "WHERE node_id='$results{linksrc}'") ->fetch;
	$srcsite = $tmp[0][0];
	$siteids{$results{linksrc}} = $srcsite;
    }else{
	$srcsite = $siteids{$results{linksrc}};
    }
    if( !exists $nodeids{$results{linksrc}} ){       
	@tmp = DBQuery("SELECT node_idx FROM site_mapping ".
		       "WHERE node_id='$results{linksrc}'") ->fetch;
	$srcnode = $tmp[0][0];
	$nodeids{$results{linksrc}} = $srcnode;
    }else{
	$srcnode = $nodeids{$results{linksrc}};
    }
    if( !exists $siteids{$results{linkdest}} ){
	@tmp = DBQuery("SELECT site_idx FROM site_mapping ".
		       "WHERE node_id='$results{linkdest}'") ->fetch;
	$dstsite = $tmp[0][0];
	$siteids{$results{linkdest}} = $dstsite;
    }else{
	$dstsite = $siteids{$results{linkdest}};
    }
    if( !exists $nodeids{$results{linkdest}} ){
	@tmp = DBQuery("SELECT node_idx FROM site_mapping ".
		       "WHERE node_id='$results{linkdest}'") ->fetch;
	$dstnode = $tmp[0][0];
	$nodeids{$results{linkdest}} = $dstnode;
    }else{
	$dstnode = $nodeids{$results{linkdest}};
    }

    if( $srcsite eq "" || $srcnode eq "" || $dstsite eq "" || $dstnode eq "" ){
	warn "No results matching node id's $results{linksrc} and/or ".
	    "$results{linkdest}. Results:\n";
	warn "srcsite=$srcsite\n";
	warn "srcnode=$srcnode\n";
	warn "dstsite=$dstsite\n";
	warn "dstnode=$dstnode\n";
	return;
    }
    my $testtype = $results{'testtype'};
    my $result   = $results{'result'};
    my $tstamp   = $results{'tstamp'};
    my $latency  = ($testtype eq "latency" ? "$result" : "NULL");
    my $loss     = ($testtype eq "loss"    ? "$result" : "NULL");
    my $bw       = ($testtype eq "bw"      ? "$result" : "NULL");

    if ($bw eq "") {
	my $src = $results{'linksrc'};
	my $dst = $results{'linkdest'};
	
	warn("BW came in as null string at $tstamp for $src,$dst\n");
	return;
    }
    if ($latency eq "") {
	my $src = $results{'linksrc'};
	my $dst = $results{'linkdest'};
	
	warn("Latency came in as null string at $tstamp for $src,$dst\n");
	return;
    }

    if ($batchsize == 0) {
	$insertions =
		"INSERT INTO pair_data (srcsite_idx, srcnode_idx, ".
		"dstsite_idx, dstnode_idx, unixstamp, ".
		"latency, loss, bw) values ";
    }
    $insertions .= ","
	if ($batchsize);

    $insertions .=
	"($srcsite, $srcnode, $dstsite, $dstnode, $tstamp, ".
	" $latency, $loss, $bw)";

    $batchsize++;
    SendBatchedInserts()
	if ($batchsize > $maxbatch);
}

sub SendBatchedInserts()
{
    if ($batchsize) {
	DBQueryWarn($insertions)
	    if (!$impotent);
	print "$insertions\n"
	    if ($debug > 2);
	$lastinsert = time();
    }
    $batchsize  = 0;
    $insertions = "";
}


#############################################################################
