#!/usr/local/bin/perl -w
#
# snmpit module for APC MasterSwitch power controllers
#
# supports new(ip), power(on|off|cyc[le],port)
#

package snmpit_apc;

$| = 1; # Turn off line buffering on output

local $debug = 0;
local $verbose = 0 + $debug;

use snmpit_lib;
use SNMP;

my $sess;
my $ip;

sub new {
  my $classname = shift;
  $ip = shift;

  while ($arg = shift ) {
    my ($var,$val) = split("=",$arg);
    ${$var} = $val;
    print "$var=${$var}\n" if $debug || $verbose;
  }

  if ($debug) {
    print "snmpit_apm module initializing... debug level $debug\n";
  }
  $SNMP::debugging = ($debug - 5) if $debug > 5;
  &SNMP::addMibDirs('/usr/local/share/snmp/mibs');
  &SNMP::addMibFiles('/usr/local/share/snmp/mibs/PowerNet-MIB.txt');
  $SNMP::save_descriptions = 1; # must be set prior to mib initialization
  SNMP::initMib();              # parses default list of Mib modules
  $SNMP::use_enums = 1;         #use enum values instead of only ints
  print "Opening SNMP session to $ip..." if $debug;
  $sess =new SNMP::Session(DestHost => $ip, Community => 'private');
  print "".(defined $sess ? "Succeeded" : "Failed")."\n" if $debug;

  my $obj = [$sess];

  bless($obj);
  return $obj;
}

sub power {
  shift;
  my $op = shift;
  my $port = shift;
  my $CtlOID = ".1.3.6.1.4.1.318.1.1.4.4.2.1.3";
  if    ($op eq "on")  { $op = "outletOn";    }
  elsif ($op eq "off") { $op = "outletOff";   }
  elsif ($op =~ /cyc/) { $op = "outletReboot";}
  if (&UpdateField(\$CtlOID,\$port,\$op)) {
    print STDERR "Outlet #$port control failed.\n";
  }
}

sub UpdateField {
  local(*OID,*port,*val)= @_;
  print "sess=$sess $OID $port $val\n" if $debug > 1;
  my $Status = 0;
  my $retval;
  print "Checking port $port of $ip for $val..." if $verbose;
  $Status = $sess->get([[$OID,$port]]);
  if (!defined $Status) {
    print STDERR "Port $port, change to $val: No answer from device\n";
    return 1;
  } else {
    print "Okay.\nPort $port was $Status\n" if $verbose;
    if ($Status ne $val) {
      print "Setting $port to $val..." if $verbose;
      $retval = $sess->set([[$OID,$port,$val,"INTEGER"]]);
	print "Set returned '$retval'" if $verbose;
	if ($retval) { return 0; } else { return 1; }
    }
    return 0;
  }
}

# End with true
1;
