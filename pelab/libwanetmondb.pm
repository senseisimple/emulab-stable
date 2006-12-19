#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#

package libwanetmondb;

use strict;
use Exporter;
use vars qw(@ISA @EXPORT);
use lib '/usr/testbed/lib';
use libtbdb;
require Exporter;

@ISA    = "Exporter";
our @EXPORT = qw ( 
                   getRows
              );
our @EXPORT_OK = qw(
                    );

my $PWDFILE = "/usr/testbed/etc/pelabdb.pwd";
my $OPSDBNAME  = "pelab";
my  $DPDBNAME  = "nodesamples";
my $DPDBHOST = "nfs.emulab.net";
my $DBUSER  = "flexlabdata";
my $OpsDataDuration = 24; #hours that data persists on the ops DB
my $dbpwd;
my $dppwd;
if (`cat $PWDFILE` =~ /^([\w]*)\s([\w]*)$/) {
    $dbpwd = $1;
    $dppwd = $2;
}
else {
    fatal("Bad characters in password!");
}
$dbpwd = '';

#
# Return an array of result row hashes based on given query string.
#   If a unixstamp range is not given, data from ops is used
#   If given unixstamp range falls within the past hour, and goes back over
#   ($OpsDataDuration-1) in time, then the query only uses the datapository
#   and WILL NOT LOOK AT data within the last hour
#
sub getRows($)
{
    my ($query) = @_;
    my ($tablename, $t0,$t1);
    my $useOps = 1;
    my @rows = ();

    if( $query =~ /from\s+(\w+)/ ){
        $tablename = $1;
    }
    if( $query =~ /unixstamp\s*>=?\s*(\d+)/ ){
        $t0 = $1;
    }
    if( $query =~ /unixstamp\s*<=?\s*(\d+)/ ){
        $t1 = $1;
    }

    if( !defined $t0 || !defined $t1 ){
        #use Ops
        $useOps = 1;
#        $t0 = time() - ($OpsDataDuration-1)*60*60;
#        $t1 = time();
    }

    #Determine whether to go to DPos or Ops databases
    if( defined $t0 &&
        $t0 < time()-($OpsDataDuration-0.5)*60*60 )
    {
        #use DataPository
        $useOps = 0;
    }else{
        #use Ops DB
        $useOps = 1;
    }



    if( $useOps ){
        TBDBConnect($OPSDBNAME, $DBUSER, $dbpwd) == 0
            or die("Could not connect to ops/pelab database!\n");
        my $query_result = DBQueryFatal($query);
        if (! $query_result->numrows) {
            warn("No results from OpsDB with query:\n$query\n");
            return @rows;
        }
        while( my $hrow = $query_result->fetchrow_hashref() ){
            push @rows, $hrow;
        }
    }else{
        TBDBConnect($DPDBNAME, $DBUSER, $dbpwd,$DPDBHOST) == 0
            or die("Could not connect to nfs/pelab database!\n");
        my $query_result = DBQueryFatal($query);
        if (! $query_result->numrows) {
            warn("No results from DataPository DB with query:\n$query\n");
            return @rows;
        }
        while( my $hrow = $query_result->fetchrow_hashref() ){
            push @rows, $hrow;
        }
    }

    return @rows;
}




1;  # DON'T REMOVE THIS
