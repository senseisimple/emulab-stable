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

#
# Turn off line buffering on output
#
$| = 1;

sub usage {
	print "Usage: $0 [-p receiveport] [-a sendport] [-e pid/eid] [-d] [-i]\n";
	return 1;
}

my $debug    = 0;
my $impotent = 0;
my ($port, $sendport, $expid);
my %opt = ();
if (!getopts("p:a:e:dih", \%opt)) {
    exit &usage;
}
if ($opt{p}) { $port = $opt{p}; } else { $port = 5051; }
if ($opt{p}) { $sendport = $opt{p}; } else { $sendport = 5050; }
if ($opt{h}) { exit &usage; }
if ($opt{e}) { $expid = $opt{e}; } else { $expid = "none"; }
if ($opt{d}) { $debug = 1; }
if ($opt{i}) { $impotent = 1; } 

if (@ARGV !=0) { exit &usage; }

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

#############################################################################
# Note a difference from tbrecv.c - we don't yet have event_main() functional
# in perl, so we have to poll. (Nothing special about the select, it's just
# a wacky way to get usleep() )
#############################################################################
#main()

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
    #check for pending received events
    my @ready = $sel->can_read(1000);
    foreach my $handle (@ready){
	$socket_rcv->recv( $inmsg, 2048 );
#	my %inhash = %{ Storable::thaw $inmsg};
	my %inhash = %{ deserialize_hash( $inmsg )};
#	foreach my $key (keys %inhash){
#	    print "key=$key\n";
#	    print "$key  \t$inhash{$key}\n";
#	}
	my ($expid, $linksrc, $linkdest, $testtype, $result, $tstamp, $index)
	    = ($inhash{expid}, $inhash{linksrc}, $inhash{linkdest},
	       $inhash{testtype}, $inhash{result}, $inhash{tstamp},
	       $inhash{index});


	print("linksrc=$linksrc\n".
	      "linkdest=$linkdest\n".
	      "testtype =$testtype\n".
	      "result=$result\n".
	      "index=$index\n".
	      "tstamp=$tstamp\n");

	if( defined $linksrc ){
	    my $socket_snd = 
	      IO::Socket::INET->new( PeerPort => $sendport,
				     Proto    => 'udp',
				     PeerAddr => "$linksrc");
	    my %ack = ( expid   => $expid,
			cmdtype => "ACK",
			index   => $index );
#	    my $ack_serial = Storable::freeze \%ack;
	    my $ack_serial = serialize_hash( \%ack );
	    $socket_snd->send($ack_serial);
       	    print "**SENT ACK**\n";
#=pod
	    if( !defined $lasttimestamp{$linksrc}{$index} ||
		$tstamp ne $lasttimestamp{$linksrc}{$index} )
	    {
		saveTestToDB(linksrc   => $linksrc,
			     linkdest  => $linkdest,
			     testtype  => $testtype,
			     result    => $result,
			     tstamp    => $tstamp );
	    }else{
		print "++++++duplicate data\n";
	    }
	    $lasttimestamp{$linksrc}{$index} = $tstamp;
#=cut
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
	    if ($debug);
	$lastinsert = time();
    }
    $batchsize  = 0;
    $insertions = "";
}


#############################################################################

#
# Custom sub to turn a hash into a string. Hashes must not contain
# the substring of $separator anywhere!!!
#
sub serialize_hash($)
{
    my ($hashref) = @_;
    my %hash = %$hashref;
    my $separator = "::";
    my $out = "";

    for my $key (keys %hash){
	$out .= $separator if( $out ne "" );
	$out .= $key.$separator.$hash{$key};
    }
    return $out;
}

sub deserialize_hash($)
{
    my ($string) = @_;
    my $separator = "::";
    my %hashout;
    
    my @tokens = split( /$separator/, $string );

    for( my $i=0; $i<@tokens; $i+=2 ){
	$hashout{$tokens[$i]} = $tokens[$i+1];
    }
    return \%hashout;
}
