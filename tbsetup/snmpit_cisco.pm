#!/usr/local/bin/perl -w
#
# snmpit module for Cisco Catalyst 6509 switches
#

package snmpit_cisco;

$| = 1; # Turn off line buffering on output

local $debug = 0;
local $block = 1;
local $verbose = 0 + $debug;

use SNMP;
use snmpit_lib;
use English;

my $sess;                     # My snmp session - initialized in new()
my $ip;

my %ifIndex;
# Holds map of ifIndex to mod.port

sub new {
  my $classname = shift;
  $ip = shift;

  while ($arg = shift ) {
    my ($var,$val) = split("=",$arg);
    ${$var} = $val;
    print "$var=${$var}\n" if $debug || $verbose;
  }

  if ($debug) {
    print "snmpit_cisco module initializing... debug level $debug\n";
  }

  #die("\n");
  $SNMP::debugging = ($debug - 2) if $debug > 2;
  my $mibpath = '/usr/local/share/snmp/mibs';
  &SNMP::addMibDirs($mibpath);
  &SNMP::addMibFiles("$mibpath/CISCO-STACK-MIB.txt",
		     "$mibpath/CISCO-VTP-MIB.txt",
		     "$mibpath/CISCO-VLAN-MEMBERSHIP-MIB.txt");
  $SNMP::save_descriptions = 1;	# must be set prior to mib initialization
  SNMP::initMib();		# parses default list of Mib modules 

  $SNMP::use_enums = 1;		#use enum values instead of only ints

  print "Opening SNMP session to $ip..." if $debug;
  $sess = new SNMP::Session(DestHost => $ip);
  print "".(defined $sess ? "Succeeded" : "Failed")."\n" if $debug;

  &readifIndex;
  my $obj = [$sess];

  bless($obj);
  return $obj;
}

my %cmdOIDs = 
  (
   "enable" => [".1.3.6.1.2.1.2.2.1.7","up"],
   "disable"=> [".1.3.6.1.2.1.2.2.1.7","down"],
   "100mbit"=> [".1.3.6.1.4.1.9.5.1.4.1.1.9","s10000000"],
   "10mbit" => [".1.3.6.1.4.1.9.5.1.4.1.1.9","s100000000"],
   "full"   => [".1.3.6.1.4.1.9.5.1.4.1.1.10","full"],
   "half"   => [".1.3.6.1.4.1.9.5.1.4.1.1.10","half"],
   "auto"   => [".1.3.6.1.4.1.9.5.1.4.1.1.9","autoDetect",
	        ".1.3.6.1.4.1.9.5.1.4.1.1.10","auto"]
  );

sub portControl {
  # returns 0 on success, # of failed ports on failure, -1 for unsupported
  shift;
  my $cmd = shift;
  my @ports = @_;
  print "portControl: $cmd -> (@ports)\n" if $debug;
  my @p = map {
    if ( /^([^-]*)-\d:(.*)$/ ) { $_ = "$1:$2"; }
    my $pnum = portnum($_);
    my $port = (split(":",$pnum))[1];
    my $i = $ifIndex{$port};
    print "$_ -> $port -> $i\n" if $debug;
    if ($cmd =~/able/) { $i; } else { $port }
  } @ports;
  if (defined $cmdOIDs{$cmd}) {
    my @oid = @{$cmdOIDs{$cmd}};
    if (@oid == 2) {
      return &UpdateField($oid[0],$oid[1],@p);
    } else {
      my $retval = 0;
      while (@oid) {
	my $myoid = shift @oid;
	my $myval = shift @oid;
	$retval += &UpdateField($myoid,$myval,@p);
      }
      return $retval;
    }
  } else {
    if ($cmd =~ /VLAN(\d+)/) {
      my $vlan = $1;
      my $rv = 0;
      foreach $port (@ports) {
	print "Setting VLAN for $port to $vlan..." if $debug;
	$rv += setPortVlan($vlan,$port);
	print ($rv ? "Failed.\n":"Succeeded.\n") if $debug;
	if ($rv) { print STDERR "VLAN change for $port failed.\n"; last; }
      }
      if ($rv) { return 1; }
      if ($vlan == 1) {
	print "Disabling @ports..." if $debug;
	if ( $rv = portControl(NULL,"disable",@ports) ) {
	  print STDERR "Port disable had $rv failures.\n";
	}
      } else {
	print "Enabling @ports..." if $debug;
	if ( $rv = portControl(NULL,"enable",@ports) ) {
	  print STDERR "Port enable had $rv failures.\n";
	}
      }
      return $rv;
    } else {
      print STDERR "Unsupported port control command '$cmd' ignored.\n";
      return -1;
    }
  }
}

sub vlanLock {
  shift;
  my $EditOp = '.1.3.6.1.4.1.9.9.46.1.4.1.1.1'; # use index 1
  my $BufferOwner = '.1.3.6.1.4.1.9.9.46.1.4.1.1.3'; # use index 1
  $RetVal = $sess->set([[$EditOp,1,"copy","INTEGER"]]);
  print "Buffer Request Set gave ",(defined($RetVal)?$RetVal:"undef."),
    "\n" if $verbose;
  $RetVal = 
    $sess->set([[$BufferOwner,1,substr(`uname -n`,0,-1),"OCTETSTR"]]);
  $RetVal = $sess->get([[$BufferOwner,1]]);
  print "Buffer Owner is $RetVal\n" if $verbose;
}

sub vlanUnlock {
  shift;
  my $EditOp = '.1.3.6.1.4.1.9.9.46.1.4.1.1.1'; # use index 1
  my $ApplyStatus = '.1.3.6.1.4.1.9.9.46.1.4.1.1.2'; # use index 1
  $RetVal = $sess->set([[$EditOp,1,"apply","INTEGER"]]);
  print "Apply set: '$RetVal'\n" if $verbose;
  my $RetVal = $sess->get([[$ApplyStatus,1]]);
  print "Apply gave $RetVal\n" if $verbose;
  while ($RetVal eq "inProgress") { 
    $RetVal = $sess->get([[$ApplyStatus,1]]);
    print "Apply gave $RetVal\n" if $verbose;
  }
  my $ApplyRetVal = $RetVal;
  if ($RetVal ne "succeeded") {
    print "Apply failed: Gave $RetVal\n" if $verbose;
    # If $u is false, release buffer
    if (!$u) {
      $RetVal = $sess->set([[$EditOp,1,"release","INTEGER"]]);
      print "Release: '$RetVal'\n" if $verbose; 
      if (! $RetVal ) {
	die("VLAN Reconfiguration Failed. No changes saved.\n");
      }
    }
  } else { 
    print "Apply Succeeded.\n" if $verbose; 
    # If I succeed, release buffer
    $RetVal = $sess->set([[$EditOp,1,"release","INTEGER"]]);
    if (! $RetVal ) {
      die("VLAN Reconfiguration Failed. No changes saved.\n");
    }
    print "Release: '$RetVal'\n" if $verbose; 
  }
  return $ApplyRetVal;
}

sub setupVlan {
  # This is to be called ONLY after vlanLock has been called successfully!
  shift;
  my $name = shift;
  my $origname = $name;
  my @vlan = @_;
  foreach $node (@vlan) {
    if (! NodeCheck($node)) { 
      print STDERR "You are not authorized to control $node.\n";
      return 1;
    }
  }
  my $okay = 0;
  my $attempts = 0;
  my $VlanType = '.1.3.6.1.4.1.9.9.46.1.4.2.1.3.1'; # vlan # is index
  my $VlanName = '.1.3.6.1.4.1.9.9.46.1.4.2.1.4.1'; # vlan # is index
  my $VlanSAID = '.1.3.6.1.4.1.9.9.46.1.4.2.1.6.1'; # vlan # is index
  my $VlanRowStatus = '.1.3.6.1.4.1.9.9.46.1.4.2.1.11.1'; # vlan # is index
  while (($okay == 0) && ($attempts <3)) {
    $attempts++;
    $name = $origname;
    print "***Okay=$okay, Attempts=$attempts (limit of 3)\n" if $debug;
    my $N = 2;
    my $RetVal = $sess->get([[$VlanRowStatus,$N]]);
    print "Row $N got '$RetVal'  " if $debug > 1;
    while ($RetVal ne '') {
      $N += 1;
      $RetVal = $sess->get([[$VlanRowStatus,$N]]);
      print "Row $N got '$RetVal'  " if $debug > 1;
    }
    print "\nUsing Row $N\n" if $debug;
    my $SAID = pack("H*",sprintf("%08x",$N+100000));
    if ( !$name ) { $name = $N; }
    print "  Creating VLAN $name as VLAN #$N: @vlan ... ";
    # Yes, this next line MUST happen all in one set command....
    $RetVal = $sess->set([[$VlanRowStatus,$N,"createAndGo","INTEGER"],
			  [$VlanType,$N,"ethernet","INTEGER"],
			  [$VlanName,$N,$name,"OCTETSTR"],
			  [$VlanSAID,$N,$SAID,"OCTETSTR"]]);
    print "",($RetVal? "Succeeded":"Failed"), ".\n";
    if (!$RetVal) {
      print STDERR "VLAN Create '$name' as VLAN $N failed.\n";
    } else {
      $RetVal = &vlanUnlock(*sess,$verbose,1); #send last as 1 to get result
      print "Got $RetVal from vlanUnlock\n" if $debug;
      # If RetVal isn't succeeded, then I can try and fix it
      my $tries = 0;
      &vlanLock(\$sess,\$v);
      if ($RetVal eq "succeeded") { $okay = 1; }
      while ($RetVal ne "succeeded") {
	$tries += 1;
	if ($RetVal eq "someOtherError" && $tries <= 2) {
	  #Duplicate name... correct and try again..
	  print STDERR "VLAN name '$name' might already be in use.\n".
	    "Renaming to '_$name'...";
	  $name = "_$name";
	  #$RetVal = $sess->set([[$VlanName,$N,"$name","OCTETSTR"]]);
	  $RetVal = $sess->set([[$VlanRowStatus,$N,"createAndGo","INTEGER"],
				[$VlanType,$N,"ethernet","INTEGER"],
				[$VlanName,$N,$name,"OCTETSTR"],
				[$VlanSAID,$N,$SAID,"OCTETSTR"]]);
	  print "",($RetVal? "Succeeded":"Failed"), ".\n";
	  $RetVal = &vlanUnlock(*sess,$verbose,1); #send last 1 to get result
	  print "Got $RetVal from vlanUnlock\n" if $debug;
	  &vlanLock(\$sess,\$verbose);
	} else {
	  # Try again from the beginning... maybe we're using a bad VLAN #
	  print STDERR "VLAN creation failed.".
	    ($attempts<3 ?" Making attempt number $attempts...\n":
	     " Third failure, giving up.\n");
	  $okay=0;
	  last;
	}
	if ($RetVal eq "succeeded") { $okay = 1; }
      }
      print "***Out of loop: Okay = $okay\n" if $debug;
      if ($okay == 0) { next; }
      # VLAN exists now - Add the ports:
      portControl(NULL,"VLAN$N",@vlan);
      #If everything went okay, break out of the loop
      $okay = 1;
    }
    print "Currently $attempts attempts.\n" if $debug;
  }
}

sub setPortVlan {
  my $vlan = shift;
  my $node = shift;
  my $PortVlanMemb = ".1.3.6.1.4.1.9.9.68.1.2.2.1.2"; #index is ifIndex
  if ($node =~ /(sh\d+)-\d(:\d)/) { $node = "$1$2"; }
  my $port = (split(":",portnum($node)))[1];
  my $IF = $ifIndex{$port};
  print "Found $node -> $port -> $IF\n" if $debug;
  if (!defined $port) {
    print STDERR "Node $node - port not found!\n";
    return 1;
  }
  $RetVal = $sess->set([[$PortVlanMemb,$IF,$vlan,'INTEGER']]);
  if (!$RetVal) {
    print STDERR "Node $node ($port) VLAN change failed.\n";
    return 1;
  }
  return 0;
}

sub removeVlan {
  shift;
  my $r = shift;
  # Check the which ports are in the vlan, and make sure that we own them
  my $VlanPortVlan = ["vlanPortVlan",0]; # index by module.port, gives vlan #
  my $RetVal = $sess->getnext($VlanPortVlan);
  my @data = @{$VlanPortVlan};
  my @ports = ();
  my $i = $sess->{DestHost};
  do {
    if ($RetVal != 1) {
      print "Got $RetVal   \t" if $debug > 1;
      print "$data[0]\t$data[1]\t$data[2]\t" if $debug > 1;
      print "('$data[1]' " if $debug > 1;
      $node = portnum("$i:$data[1]");
      print "== $node)\n" if $debug > 1;
      if ($RetVal == $r) {
	push(@ports,$node);
	if (!NodeCheck($node)) {
	  print STDERR "You are not authorized to control $node.\n";
	  return 1;
	}
      }
      #do the getnext at the end, because if we're on the last, the next
      #one is junk to all the processing instructions...
    }
    $RetVal = $sess->getnext($VlanPortVlan);
    @data = @{$VlanPortVlan};
  } while ( $data[0] =~ /^vlanPortVlan/ ) ;
  my $VlanRowStatus = '.1.3.6.1.4.1.9.9.46.1.4.2.1.11.1'; # vlan # is index
  if ($r == 1) {
    print STDERR "VLAN #$r is the Control VLAN, and cannot be removed.\n";
  } else {
    print "  Removing VLAN #$r ... ";
    my $RetVal = $sess->set([[$VlanRowStatus,$r,"destroy","INTEGER"]]);
    print ($RetVal ? "Succeeded.\n" : "Failed.\n");
    if (! defined $RetVal) { 
      print STDERR "VLAN #$r does not exist on this switch.\n";
      return;
    }
    portControl(NULL,"VLAN1",@ports);
  }
}

sub UpdateField {
  # returns 0 on success, # of failed ports on failure
  print "UpdateField: '@_'\n" if $debug;
  #$sess = shift;
  #$sess = $$sess;		# unreference it...
  my ($OID,$val,@ports)= @_;
  my $Status = 0;
  my $err = 0;
  my $retval;
  foreach my $port (@ports) {
    my $trans = $ifIndex{$port};
    if (defined $trans) {
      if (defined (portnum("$ip:$trans"))) {
	$trans = "$trans,".portnum("$ip:$trans");
      } else {
	$trans = "$trans,".portnum("$ip:$port");
      }
    } else {
      $trans = "???";
    }
    print "Checking port $port ($trans) for $val..." if $debug;
    $Status = $sess->get([[$OID,$port]]);
    if (!defined $Status) {
      print STDERR "Port $port ($trans), change to $val: No answer from device\n";
      return -1;		# return error
    } else {
      print "Okay.\n" if $debug;
      print "Port $port was $Status\n" if $verbose;
      if ($Status ne $val) {
	print "Setting $port to $val..." if $verbose;
	# Don't use async
	$retval = $sess->set([[$OID,$port,$val,"INTEGER"]]);
	print "Set returned '$retval'\n" if (defined $retval) && $debug;
	if ($block) {
	  $n = 0;
	  while ($Status ne $val) {
	    $Status=$sess->get([[$OID,$port]]);
	    print "Value for $port was ",$Status,"\n" if $debug;
	    select (undef, undef, undef, .25); # wait .25 seconds
	    $n++;
	    if ($n > 20) {
	      $err++;
	      print "Timing out..." if $verbose;
	      last;
	    }
	  }
	  print "Okay.\n" if $verbose;
	} else {
	  print "\n" if $verbose;
	}
      }
    }
  }
  # returns 0 on success, # of failed ports on failure
  $err;
}

#used for numerical sort...
sub num { $a <=> $b; }

#used for alphanumerical sort...
sub alphanum { 
  $a =~ /^([a-z]*)([0-9]*)/;
  $a_let = ($1 || "");
  $a_num = ($2 || 0);
  $b =~ /^([a-z]*)([0-9]*)/;
  $b_let = ($1 || "");
  $b_num = ($2 || 0);
  if ($a_let eq $b_let) {
    return $a_num <=> $b_num;
  } else {
    return $a_let cmp $b_let;
  }
  return 0;
}
#used for alphanumerical sort...
sub alphanum2 { 
  $a =~ /^([a-z]*)([0-9]*):?([0-9]*)/;
  $a_let = ($1 || "");
  $a_num = ($2 || 0);
  $a_num2 = ($3 || 0);
  $b =~ /^([a-z]*)([0-9]*):?([0-9]*)/;
  $b_let = ($1 || "");
  $b_num = ($2 || 0);
  $b_num2 = ($3 || 0);
  if ($a_let eq $b_let) {
    if ($a_num == $b_num) {
      return $a_num2 <=> $b_num2;
    } else {
      return $a_num <=> $b_num;
    }
  } else {
    return $a_let cmp $b_let;
  }
  return 0;
}

sub listVlans {
  shift;
  my %Names = ();
  my %Members = ();
  my @data = ();
  my $mac = "";
  my $node= "";
  my @vlan = ();
  print "Getting VLAN info...\n" if $debug;
  #my $VlanIndex = ["vlanIndex",0]; # index by vlan #
  # We don't need VlanIndex really...
  my $VlanName = ["vtpVlanName",0]; # index by 1.vlan #
  my $VlanPortVlan = ["vlanPortVlan",0]; # index by module.port, gives vlan #
  #do one to get the first field...
  my $RetVal = $sess->getnext($VlanName);
  if (!defined $RetVal) { die("No response from device\n"); }
  @data = @{$VlanName};
  my $num = 0;
  do {
    $data[1] =~ /\.(.*)/;
    $num = $1;
    print "Got $RetVal   \t" if $debug;
    print "$data[0]\t$data[1] ($num)\t$data[2]\n" if $debug;
    if ( !$Names{$num} && $num < 1000 ) { $Names{$num} = $RetVal; }
    #do the getnext at the end, because if we're on the last, the next
    #one is junk to all the processing instructions...
    $RetVal = $sess->getnext($VlanName);
    @data = @{$VlanName};
  } while ( $data[0] =~ /^vtpVlanName/ ) ;
  $RetVal = $sess->getnext($VlanPortVlan);
  @data = @{$VlanPortVlan};
  my $i = $sess->{DestHost};
  do {
    if ($RetVal != 1) {
      print "Got $RetVal   \t" if $debug;
      print "$data[0]\t$data[1]\t$data[2]\t" if $debug;
      my @vlan=();
      if (defined (@{$Members{$data[2]}})) { @vlan =@{$Members{$data[2]}}; }
      print "('$data[1]' " if $debug;
      ($node = portnum("$ip:$data[1]")) || ($node = "?");
      print "== $node)\n" if $debug;
      push(@vlan, $node) if ! ($node =~ /^empty/);
      $Members{$data[2]} = \@vlan;
      #do the getnext at the end, because if we're on the last, the next
      #one is junk to all the processing instructions...
    }
    $RetVal = $sess->getnext($VlanPortVlan);
    @data = @{$VlanPortVlan};
  } while ( $data[0] =~ /^vlanPortVlan/ );
  my @list = ();
  foreach $id (sort num keys %Names) {
    push @list, join("\t", $id, $Names{$id},
		     (defined(@{$Members{$id}}) ?
		      join(" ",sort (@{$Members{$id}})) : ""));
  }
  print join("\n",@list),"\n" if $debug > 1;
  return @list;
}

sub showPorts {
  shift;
  my %Able = ();
  my %Link = ();
  my %speed = ();
  my %duplex = ();
  my $ifTable = ["ifAdminStatus",0];
  my @data=();
  my $port;
  #do one to get the first field...
  print "Getting port information...\n" if $debug;
  do {{
    $sess->getnext($ifTable);
    @data = @{$ifTable};
    if ($data[0] eq "ifLastChange") {
      $ifTable = ["portAdminSpeed",0];
      $sess->getnext($ifTable);
      @data = @{$ifTable};
    }
    if (! defined $ifIndex{$data[1]}) { next; }
    $port = portnum("$ip:$data[1]") || portnum("$ip:".$ifIndex{$data[1]});
    if (! defined $port) { next; }
    print "$port\t$data[0]\t$data[2]\n" if $debug > 1;
    if    ($data[0]=~/AdminStatus/){ $Able{$port}=($data[2]=~/up/?"yes":"no");}
    elsif ($data[0]=~/OperStatus/) { $Link{$port}= $data[2];}
    elsif ($data[0]=~/AdminSpeed/) {$speed{$port}= $data[2];}
    elsif ($data[0]=~/Duplex/)     {$duplex{$port}=$data[2];}
    # Insert stuff here to get speed, duplex
  }} while ( $data[0] =~ /^(i(f)(.*)Status)|(port(AdminSpeed|Duplex))$/) ;
  print "Port Configuration, Testbed Switch $ip\n";
  print "Port     \tEnabled\tLink\tSpeed\t\tDuplex\n";
  print "---------------------------------------------------------------\n";
  foreach my $id ( sort alphanum2 keys (%Able) ) {
    my $vlan;
    if (! defined ($speed{$id}) ) { $speed{$id} = " "; }
    if (! defined ($duplex{$id}) ) { $duplex{$id} = " "; }
    $speed{$id} =~ s/s([10]+)000000/${1}Mbps/;
    print $id,"   \t";
    print $Able{$id},"\t",$Link{$id},"\t";
    print $speed{$id},"  \t",$duplex{$id},"\n";
  }
}

sub getStats {
  shift;
  my $port;
  my $MinPorts = 10;
  my $MaxPorts = 393;
  my $ifTable = ["ifInOctets",0];
  my %inOctets=();
  my %inUcast=();
  my %inNUcast=();
  my %inDiscard=();
  my %inErr=();
  my %inUnkProt=();
  my %outOctets=();
  my %outUcast=();
  my %outNUcast=();
  my %outDiscard=();
  my %outErr=();
  my %outQLen=();
  my @data=();
  #do one to get the first field...
  print "(This may take a minute... literally)\n";
  $sess->getnext($ifTable);
  do {{
    @data = @{$ifTable};
    $port = $ifIndex{$data[1]};
    if (defined $port) { $port = portnum("$ip:$port"); }
    if (! defined $port) { $sess->getnext($ifTable); next; }
    print "$port\t$data[0]\t$data[2]\n" if $debug > 1;
    if (($data[1] >= $MinPorts) && ($data[1] <= $MaxPorts)) {
      if    ($data[0]=~/InOctets/)  {$inOctets{$port}  = $data[2];}
      elsif ($data[0]=~/InUcast/)   {$inUcast{$port}   = $data[2];}
      elsif ($data[0]=~/InNUcast/)  {$inNUcast{$port}  = $data[2];}
      elsif ($data[0]=~/InDiscard/) {$inDiscard{$port} = $data[2];}
      elsif ($data[0]=~/InErrors/)  {$inErr{$port}     = $data[2];}
      elsif ($data[0]=~/InUnknownP/){$inUnkProt{$port} = $data[2];}
      elsif ($data[0]=~/OutOctets/) {$outOctets{$port} = $data[2];}
      elsif ($data[0]=~/OutUcast/)  {$outUcast{$port}  = $data[2];}
      elsif ($data[0]=~/OutNUcast/) {$outNUcast{$port} = $data[2];}
      elsif ($data[0]=~/OutDiscard/){$outDiscard{$port}= $data[2];}
      elsif ($data[0]=~/OutErrors/) {$outErr{$port}    = $data[2];}
      elsif ($data[0]=~/OutQLen/)   {$outQLen{$port}   = $data[2];}
    }
    $sess->getnext($ifTable);
  }} while ( $data[0] =~ /^i[f](In|Out)/) ;
  print "Port Statistics, Testbed Switch $ip\n";
  print "                        InUcast    InNUcast  In       In     Unknown              OutUcast   OutNUcast Out      Out    OutQueue\n";
  print "Interface  InOctets     Packets    Packets   Discards Errors Proto.  OutOctets    Packets    Packets   Discards Errors Length\n";
  print "------------------------------------------------------------------------------------------------------------------------------------\n"; 
  format stats =
#nterface  InOctets     Packets    Packets   Discards Errors Proto.  Out Octets   Packets    Packets   Discards Errors Length
@<<<<<<<<  @<<<<<<<<<<< @<<<<<<<<< @<<<<<<<< @<<<<<<< @<<<<< @<<<<<< @<<<<<<<<<<< @<<<<<<<<< @<<<<<<<< @<<<<<<< @<<<<< @<<<<<<<
$port,$inOctets{$port},$inUcast{$port},$inNUcast{$port},$inDiscard{$port},$inErr{$port},$inUnkProt{$port},$outOctets{$port},$outUcast{$port},$outNUcast{$port},$outDiscard{$port},$outErr{$port},$outQLen{$port}
.
  $FORMAT_NAME = "stats";
  foreach $port ( sort alphanum2 keys (%inOctets) ) {
    write;
  }
  $FORMAT_NAME = "STDOUT"; # Go back to normal format
}

sub readifIndex {
  my @list=();
  my $modport;
  # For now, just look on the switch I'm interested in...
  # Later, we might have to see which switch the port is on and check that one
  my $portIfIndex = [".1.3.6.1.4.1.9.5.1.4.1.1.11",0];
  $rv = $sess->getnext($portIfIndex);
  my @data = @{$portIfIndex};
  do {
    $node = portnum("$ip:$data[1]");
    if (defined $node) {
      print "Got $rv   \t$data[0]\t$data[1]\t$data[2]\t" if $debug > 2;
      print "('$data[1]' == $node)\n" if $debug > 2;
      $modport=$data[1];
      print "($modport == $rv == $node)\n" if $debug > 1;
      $ifIndex{$modport} = $rv;
      $ifIndex{$rv} = $modport;
    }
    $rv = $sess->getnext($portIfIndex);
    @data = @{$portIfIndex};
  } while ( $data[0] =~ /^portIfIndex/ );
}

# End with true
1;

