#!/usr/local/bin/perl -w
#
# snmpit-lib module
#
# supports the following methods:
# macport(pcX:Y || MAC)       return MAC || pcX:Y address respectively
# portnum(pcX:Y || node:port) return node:mod.port or pcX:Y
# Dev(name || IP)             return IP/name
# NodeCheck(pcX:Y)	      return true (1) if okay, false (0) if it failed
# tableVlans(pid,eid)         returns hash: key=id, val=members

package snmpit_lib;

use Exporter;
@ISA = ("Exporter");
@EXPORT = qw( macport portnum Dev NodeCheck tableVlans );

use English;
use Mysql;

my $debug = 0;

my $dbh;
my $sth;

my %Devices=();
# %Devices maps device names to device IPs

my %Interfaces=();
# %Interfaces maps pcX:Y<==>MAC

my %Ports=();
# %Ports maps pcX:Y<==>switch:port

my $self = (getpwuid($UID))[0]
  || die "Cannot figure out who you are!\n";
print "User is '$self' (UID $UID)\n" if $debug;

my $admin=0;

my $init = 0 ; # Start out uninitialized

sub init {
  $DBNAME = shift;
  $dbh = Mysql->connect("localhost",$DBNAME,"script","none");
  print "Database initialized.\n" if $debug;
  &ReadTranslationTable;
  return 0;
}

sub macport {
  my $val = shift || "";
  return $Interfaces{$val};
}

sub portnum {
  my $val = shift || "";
  return $Ports{$val};
}

sub Dev {
  my $val = shift || "";
  return $Devices{$val};
}

sub ReadTranslationTable {
  # This function fills in %Interfaces and %Ports
  # They hold pcX:Y<==>MAC and pcX:Y<==>switch:port respectively
  my $name="";
  my $mac="";
  my $switchport="";

  print "FILLING %Interfaces\n" if $debug;
  $sth = $dbh->query("select * from interfaces;");
  while ( @_ = $sth->fetchrow_array()) {
    $name = "$_[0]:$_[1]";
    if ($_[2] != 1) {$name .=$_[2]; }
    $mac = "$_[3]";
    $Interfaces{$name} = $mac;
    $Interfaces{$mac} = $name;
    print "Got $mac <==> $name\n" if $debug > 1;
  }


  print "FILLING %Devices\n" if $debug;
  $sth = $dbh->query("select i.node_id,i.IP from interfaces as i ".
		     "left join nodes as n on n.node_id=i.node_id ".
		     "where n.type!='pc' and n.type!='shark'");
  while ( @_ = $sth->fetchrow_array()) {
    $name = "$_[0]";
    $ip = "$_[1]";
    $Devices{$name} = $ip;
    $Devices{$ip} = $name;
    print "Devices: $name <==> $ip\n" if $debug > 1;
  }

  print "FILLING %Ports\n" if $debug;
  $sth = $dbh->query("select node_id1,card1,port1,node_id2,card2,port2 ".
		     "from wires where node_id2 like 'cisco%';");
  while ( @_ = $sth->fetchrow_array()) {
    $name = "$_[0]:$_[1]";
    print "Name='$name'\t" if $debug > 2;
    if (defined ($Devices{$_[3]}) ) { $_[3] = $Devices{$_[3]}; }
    else { 
      print STDERR "SNMPIT: Warning: Device '$_[3]' from database unknown\n";
      $_[3] = $Devices{"cisco"}; 
    }
    print "Dev='$_[3]'\t" if $debug > 2;
    $switchport = join(":",($_[3],$_[4]));
    $switchport .=".$_[5]";
    print "switchport='$switchport'\n" if $debug > 2;
    $Ports{$name} = $switchport;
    $Ports{$switchport} = $name;
    print "READ: '$name' <==> '$switchport'\n" if $debug > 1;
  }
}

sub NodeCheck {
  local($node) = @_;

  if ($admin) { return 1; }

  print "Checking authorization for $node..." if $debug;

  # If my uid or my euid are root, its okay
  if ( ($UID == 0) || ($EUID == 0) ) {
    $admin = 1;
    print "(Root!) Okay from now on.\n" if $debug;
    return 1;
  }

  my $cmd =
    "select uid,admin from users where uid='$self' and admin=1;";
  print "$cmd\n" if $debug > 1;
  $sth = $dbh->query($cmd);

  if ($sth->numrows > 0) {
    $admin = 1;
    print "(Admin!) Okay from now on.\n" if $debug;
    return 1;
  }

  if ( $node =~ /^([^:]*)/ ) { $node=$1; }
  if ( $node =~ /(sh\d+)/ ) { $node= $1."-1"; }

  $cmd =
    "select uid, node_id from reserved as r left join proj_memb as pm ".
    "on pm.pid=r.pid where node_id='$node' and uid='$self';";
  print "$cmd\n" if $debug > 1;
  $sth = $dbh->query($cmd);

  if ($sth->numrows > 0) {
    print "Okay.\n" if $debug;
    return 1;
  }

  print "Denied.\n" if $debug;
  print STDERR "Authorization Failed for control to node $node.\n";
  return 0;
}

sub tableVlans {
  my $pid = shift;
  my $eid = shift;
  #returns hash, key=id, val=members
  my %table = ();
  $sth = $dbh->
    query("select id,members from vlans where pid='$pid' and eid='$eid'");
  while (@row = $sth->fetchrow_array()) {
    $table{$row[0]} = $row[1];
  }
  return %table;
}

# End with true
1;

