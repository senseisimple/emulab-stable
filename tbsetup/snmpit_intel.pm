#!/usr/local/bin/perl -w
#
# snmpit module for Intel EtherExpress 510T switches
#

package snmpit_intel;

# Special Note:
# Because this code is not urgently needed, and it is unknown if it ever
# will be needed again, it has not been debugged since moving it into 
# this module. So don't count on it. It will need to be debugged before
# actually using it for anything useful.

$| = 1;				# Turn off line buffering on output

my $debug = 0;
my $block = 0;
my $confirm = 1;
my $verbose = 0;

use SNMP;
use snmpit_lib;

my $sess;			# My snmp session - initialized in new()

sub new {
  my $ip = shift;

  $SNMP::debugging = ($debug - 5) if $debug > 5;
  &SNMP::addMibDirs('/usr/local/share/snmp/mibs');
  &SNMP::addMibFiles('/usr/local/share/snmp/mibs/INTEL-GEN-MIB.txt', 
		     '/usr/local/share/snmp/mibs/INTEL-S500-MIB.txt',
		     '/usr/local/share/snmp/mibs/INTEL-VLAN-MIB.txt');
  $SNMP::save_descriptions = 1; # must be set prior to mib initialization
  SNMP::initMib();		# parses default list of Mib modules 

  $SNMP::use_enums = 1;		#use enum values instead of only ints

  print "Opening SNMP session to $ip..." if $debug;
  $sess = new SNMP::Session(DestHost => $ip);
  print "".(defined $sess ? "Succeeded" : "Failed")."\n" if $debug;

  my $obj = \$sess;

  bless($obj);
  return $obj;
}

my %cmdOIDs = 
  {
   "enable" => (".1.3.6.1.2.1.2.2.1.7","up"),
   "disable"=> (".1.3.6.1.2.1.2.2.1.7","down"),
   "100mbit"=> (".1.3.6.1.4.1.343.6.10.2.4.1.10.1.1","speed100Mbit"),
   "10mbit" => (".1.3.6.1.4.1.343.6.10.2.4.1.10.1.1","speed10Mbit"),
   "full"   => (".1.3.6.1.4.1.343.6.10.2.4.1.11.1.1","full"),
   "half"   => (".1.3.6.1.4.1.343.6.10.2.4.1.11.1.1","half"),
   "auto"   => (".1.3.6.1.4.1.343.6.10.2.4.1.12.1.1","auto")
  };

sub portControl {
  # returns 0 on success, # of failed ports on failure, -1 for unsupported
  my $sess = shift;
  $sess = $$sess;		# unreference it...
  my $cmd = shift;
  my @ports = @_;
  my @p = map { portnum($_) } @ports;
  my @oid = @cmdOIDs{$cmd};
  if (defined @oid) {
    return &UpdateField(\$oid[0],\$oid[1],\@p);
  } else {
    print STDERR "Unsupported port control command '$cmd' ignored.\n";
    return -1;
  }
}

sub vlanLock {
  my $sess = shift;
  $sess = $$sess;		# unreference it...
  my $TokenOwner = '.1.3.6.1.4.1.343.6.11.4.5';
  my $TokenReq = '.1.3.6.1.4.1.343.6.11.4.6';
  my $TokenReqResult = '.1.3.6.1.4.1.343.6.11.4.7';
  my $TokenRelease = '.1.3.6.1.4.1.343.6.11.4.8';
  my $TokenReleaseResult = '.1.3.6.1.4.1.343.6.11.4.9';
  #The zeros and ones are a magic number it needs...
  my $Num = pack("C*",0,0,0,0,1,1);
  my $RetVal = 0;
  my $tries = 0;
  while ($RetVal ne "success" && $tries < 10) {
    $tries += 1;
    $sess->set([[$TokenReq,0,$Num,"OCTETSTR"]]);
    $RetVal = $sess->get([[$TokenReqResult,0]]);
    while ($RetVal eq "notReady") {
      $RetVal = $sess->get([[$TokenReqResult,0]]);
      print "*VLAN Token Claim Result is $RetVal\n" if ($verbose);
      select (undef, undef, undef, .25); #wait 1/4 second
    }
    print "VLAN Token Claim Result is $RetVal\n" if ($verbose);
    if ($RetVal ne 'success') {
      my $Owner = $sess->get([[$TokenOwner,0]]);
      if ($Owner ne "0.0.0.0") { 
	$tries = 10;
      } else {
	print STDERR time,
	  "Try #$tries - Result is $RetVal - Waiting 2 seconds\n" if $debug;
	select (undef, undef, undef, 2); #wait 2 seconds
      }
    }
  }
  if ($RetVal ne 'success') {
    my $Owner = $sess->get([[$TokenOwner,0]]);
    print STDERR "VLAN Token Claim Result is $RetVal\n";
    die("Can't edit VLANs: Token taken by $Owner\n");
  }
}

sub vlanUnlock {
  my $sess = shift;
  $sess = $$sess;		# unreference it...
  my $TokenRelease = '.1.3.6.1.4.1.343.6.11.4.8';
  my $TokenReleaseResult = '.1.3.6.1.4.1.343.6.11.4.9';
  my $TokenConfirmState = '.1.3.6.1.4.1.343.6.11.1.18';
  my $save = ($confirm ? "saveWithConfirmOption" : "save");
  print "Releasing Token with $save command\n" if ($verbose);
  $sess->set([[$TokenRelease,0,$save,"INTEGER"]]);
  my $RetVal = $sess->get([[$TokenReleaseResult,0]]);
  print "VLAN Configuration Save Result is $RetVal\n" if ($verbose);
  while ($RetVal eq "notReady") {
    $RetVal = $sess->get([[$TokenReleaseResult,0]]);
    print "VLAN Configuration Save Result is $RetVal\n" if ($verbose);
    select (undef, undef, undef, .25); #wait 1/4 second
  }
  if ($confirm) {
    $RetVal = $sess->get([[$TokenConfirmState,0]]);
    print "VLAN Configuration Confirm Result is $RetVal\n" if ($verbose);
    while ($RetVal eq "notReady") {
      sleep(2);
      $RetVal = $sess->get([[$TokenConfirmState,0]]);
      print "VLAN Configuration Confirm Result is $RetVal\n" if ($verbose);
    }
    if ($RetVal eq "ready") {
      $RetVal = $sess->set([[$TokenConfirmState,0,"confirm","INTEGER"]]);
      print "VLAN Configuration Confirm Result is $RetVal\n" if ($verbose);
    }
    while (!($RetVal =~ /Conf/i)) {
      $RetVal = $sess->get([[$TokenConfirmState,0]]);
      print "VLAN Configuration Confirm Result is $RetVal\n" if ($verbose);
    }	
    if ($RetVal ne "confirmedNewConf") {
      die("VLAN Reconfiguration Failed. No changes saved.\n");
    }
  } else {
    if ($RetVal ne 'success') {
      print "VLAN Configuration Save Result is $RetVal\n" if ($verbose);
      die("$RetVal VLAN Reconfiguration Failed. No changes saved.\n");
    }
  }
}

sub setupVlan {
  # This is to be called ONLY after vlanLock has been called successfully!
  my $sess = shift;
  $sess = $$sess;		# unreference it...
  my $name = shift;
  my @vlan = @_;
  foreach $mac (@vlan) {
    my $node = $Interfaces{$mac};
    if (! NodeCheck($node)) { 
      print STDERR "You are not authorized to control $node.\n";
      return 1;
    }
  }

  my $NextVLANId = '.1.3.6.1.4.1.343.6.11.1.6';
  my $Vlan = $sess->get([[$NextVLANId,0]]);
  my $CreateOID = ".1.3.6.1.4.1.343.6.11.1.9.1.3";
  my $RetVal = "Undef.";
  if ( !$name 
       #Temporary fix: if its a lX-Y name from assign.tcl, ignore it
       || ($name =~ /^l(\d)+-(\d)+/)
       #End of Temp fix
     ) {
    $name = $Vlan;
  }
  print "  Creating VLAN $name as VLAN #$Vlan: @vlan ... ";
  $RetVal = $sess->set([[$CreateOID,$Vlan,$name,"OCTETSTR"]]);
  print "",($RetVal? "Succeeded":"Failed"),".\n";
  if (! defined ($RetVal) ) {
    &vlanUnlock(*sess,$verbose,1);
    die("VLAN name \"$name\" not unique.\n");
  }
  my @x;
  my $n=0;
  while (@vlan != 0 && $n < @vlan) {
    my $i=0;
    while ($i < 6 ) {
      $x[$i] = hex ("0x".substr($vlan[$n],2*$i,2) );
      $i++;
    }
    my $MacObjOID = ".1.3.6.1.4.1.343.6.11.1.10.1.3.$Vlan." . 
      "$x[0].$x[1].$x[2].$x[3].$x[4].$x[5]";
    print "    Adding MAC Address $vlan[$n] ".
      "($Vlan.$x[0].$x[1].$x[2].$x[3].$x[4].$x[5])... ";
    $RetVal = $sess->set([[$MacObjOID,0,$vlan[$n],"OCTETSTR"]]);
    print "",($RetVal? "Succeeded":"Failed"), ".\n";
    $n++;
  }
}

sub removeVlan {
  my $sess = shift;
  $sess = $$sess;		# unreference it...
  my $r = shift;
  my $DeleteOID = ".1.3.6.1.4.1.343.6.11.1.9.1.4";
  my $RetVal = "Undef.";
  print "  Removing VLAN #$r ... ";
  if ($r == 24) {
    print STDERR "VLAN #$r is the Control VLAN, and cannot be removed.\n";
  } else {
    $RetVal = $sess->set([[$DeleteOID,$r,"delete","INTEGER"]]);
    print "",($RetVal? "Succeeded":"Failed"),".\n";
    if (! defined ($RetVal) ) {
      print STDERR "VLAN #$r does not exist on this switch.\n";
    }
  }
}

#
# SUB UpdateField----------------------
#

sub UpdateField {
  # returns 0 on success, # of failed ports on failure
  my $sess = shift;
  $sess = $$sess;		# unreference it...
  local(*OID,*val,*ports)= @_;
  my $Status = 0;
  my $retval;
  my $i = $sess->{DestHost};
  foreach my $port (@ports) {
    my $trans = $ifIndex{$port};
    if (defined $trans) {
      if (defined ($Ports{"$i:$trans"})) {
	$trans = "$trans,".$Ports{"$i:$trans"};
      } else {
	$trans = "$trans,".$Ports{"$i:$port"};
      }
    } else { $trans = "???"; }
    print "Checking port $port ($trans) for $val..." if $verbose;
    $Status = $sess->get([[$OID,$port]]);
    if (!defined $Status) {
      print STDERR "Port $port ($trans), change to $val: No answer from device\n";
      return 1;			# Return error
    } else {
      print "Okay.\nPort $port was $Status\n" if $verbose;
      if ($Status ne $val) {
	print "Setting $port to $val..." if $verbose;
	# The empty sub {} is there to force it into async mode
	$sess->set([[$OID,$port,$val,"INTEGER"]],sub {});
	if ($block) {
	  while ($Status ne $val) { 
	    $Status=$sess->get([[$OID,$port]]);
	    print "Value for $port was ",$Status,"\n" if ($verbose);
	  }
	  print "Okay.\n";
	} else { print "\n"; }
      }
    }
  }
  if ( (!$block) && $confirm ) {
    my $loops=0;
    my $max_loops=10;
    my %notdone=();
    my @done=();
    foreach my $port (@ports) {
      $Status=$sess->get([[$OID,$port]]);
      print "Value for $port was ",$Status,"\n" if ($verbose);
      if ($Status ne $val) { $notdone{$port}=1; }
    }
    while ( %notdone && $loops < $max_loops ) {
      if ($loops > 5) {
	sleep($loops-5);
      }
      foreach my $port (sort num keys(%notdone)) {
	$Status=$sess->get([[$OID,$port]]);
	print "Value for $port was ",$Status,"\n" if ($verbose);
	if ($Status eq $val) {
	  push(@done,$port);
	}
      }
      foreach my $i (@done) {
	delete $notdone{$i};
      }
      ;
      $loops++;
    }
    if ($loops==$max_loops) {
      my $err = 0;
      foreach my $port (sort num keys(%notdone)) {
	$err++;
	print STDERR "Port $port Change not verified!\n";
      }
      return $err;
    }
  }
  # returns 0 on success, # of failed ports on failure
  return 0;
}

#used for numerical sort...
sub num { $a <=> $b; }

#used for alphanumerical sort...
sub alphanum { 
  $a =~ /^([a-z]*)([0-9]*)/;
  $a_let = $1;
  $a_num = $2;
  $b =~ /^([a-z]*)([0-9]*)/;
  $b_let = $1;
  $b_num = $2;
  if ($a_let eq $b_let) {
    return $a_num <=> $b_num;
  } else {
    return $a_let cmp $b_let;
  }
  return 0;
}

sub listVlans {
  my $sess = shift;
  $sess = $$sess;		# unreference it...
  my %Names = ();
  my %Members = ();
  my @data = ();
  my $mac = "";
  my $node= "";
  my @vlan = ();
  my $field = ["vlanPolicyVlanTable",0];
  #do one to get the first field...
  $sess->getnext($field);
  do {
    @data = @{$field};
    print "$data[0]\t$data[1]\t$data[2]\n" if $debug;
    if ($data[0] =~ /policyVlanName/) {
      $Names{$data[1]} = $data[2];
    }
    if ($data[0] =~ /MacRuleVlanId/) {
      my @vlan=();
      if (defined (@{$Members{$data[2]}})) {
	@vlan =@{$Members{$data[2]}};
      }
      $_= sprintf("%d:%02x%02x%02x%02x%02x%02x", split(/\./,$data[1]));
      @_= split(/:/,$_);
      $mac = $_[1];
      if (defined ( $Interfaces{$mac} ) ) {
	$node = $Interfaces{$mac};
      } else {
	$node = $mac;
      }
      push(@vlan, $node);
      $Members{$data[2]} = \@vlan;
    }
    #do the getnext at the end, because if we're on the last, the next
    #one is junk to all the processing instructions...
    $sess->getnext($field);
  } while ( $data[0] =~ /^(policyVlan)|(policyMacRuleVlanId)/) ;
  print "ID  Name\t\t\t    Members of VLAN\n";
  print "--------------------------------------------------\n";
  my $id;
  my $memberstr;
  # I'm trying out using formats. See perldoc perlform for details.
  format = 
@<< @<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< ^<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
($id),($Names{$id}),                $memberstr
~                                   ^<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                                    $memberstr
~                                   ^<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                                    $memberstr
~                                   ^<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                                    $memberstr
~                                   ^<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                                    $memberstr
~                                   ^<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                                    $memberstr
~                                   ^<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                                    $memberstr
~                                   ^<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                                    $memberstr
~                                   ^<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                                    $memberstr
~                                   ^<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                                    $memberstr
~                                   ^<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                                    $memberstr
~                                   ^<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                                    $memberstr
~                                   ^<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                                    $memberstr
~                                   ^<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                                    $memberstr
.
  foreach $id ( sort num keys (%Names) ) {
    $memberstr = (defined(@{$Members{$id}}) ?
                  join("  ",sort alphanum (@{$Members{$id}})) : "");
    write;
  }
}

sub ShowPorts {
  my $sess = shift;
  $sess = $$sess;		# unreference it...
  my %Able = ();
  my %Link = ();
  my %auto = ();
  my %speed = ();
  my %duplex = ();
  my %vlanNames = ();
  my %portNames = ();
  my $MinPorts = 1;
  my $MaxPorts = 24;
  my $ifTable = ["ifAdminStatus",0];
  my @data=();
  #do one to get the first field...
  $sess->getnext($ifTable);
  do {
    @data = @{$ifTable};
    print "$data[0]\t$data[1]\t$data[2]\n" if $debug;
    if (($data[1] >= $MinPorts) && ($data[1] <= $MaxPorts)) {
      if ($data[0] =~ /AdminStatus/) { 
	$Able{$data[1]} = ($data[2]=~/up/ ? "yes" : "no");
	print "(Last was admin=$Able{$data[1]})\n" if $debug;
      }
      if ($data[0] =~ /OperStatus/) { 
	$Link{$data[1]} = $data[2]; 
	print "(Last was oper=$Link{$data[1]})\n" if $debug;
      } 
    }
    #do the getnext at the end, because if we're on the last, the next
    #one is junk to all the processing instructions...
    $sess->getnext($ifTable);
  } while ( $data[0] =~ /^i(f)(.*)Status$/) ;
  my $portConf = ["portConfSpeed",0];
  #do one to get the first field...
  $sess->getnext($portConf);
  do {
    @data = @{$portConf};
    $data[1] =~ s/\./:/g;
    $data[1] = (split(":",$data[1]))[2];
    print "$data[0]\t$data[1]\t$data[2]\n" if $debug;
    if (($data[1] >= $MinPorts) && ($data[1] <= $MaxPorts)) {
      if ($data[0] =~ /Speed/) {
	$data[2] =~ s/speed//;
	$data[2] =~ s/autodetect/\(auto\)/i;
	$speed{$data[1]} = $data[2]; 
      }
      if ($data[0] =~ /Duplex/) { 
	$data[2] =~ s/autodetect/\(auto\)/i;
	$duplex{$data[1]} = $data[2]; 
      } 
      if ($data[0] =~ /AutoNeg/) {
	$auto{$data[1]} = $data[2];
      } 
    }
    #do the getnext at the end, because if we're on the last, the next
    #one is junk to all the processing instructions...
    $sess->getnext($portConf);
  } while ( $data[0] =~ /^portConf(Speed|Duplex|AutoNeg)$/);
  my $field = ["vlanPolicyVlanTable",0];
  my $mac="";
  my $id=0;
  #do one to get the first field...
  $sess->getnext($field);
  do {
    @data = @{$field};
    print "$data[0]\t$data[1]\t$data[2]\n" if $debug;
    if ($data[0] =~ /policyVlanName/) {
      $vlanNames{$data[1]} = $data[2];
    }
    if ($data[0] =~ /MacRuleVlanId/) { 
      my @vlan=();
      if (defined ( @{$MACs{$data[2]}} ) ) {
	@vlan =@{$MACs{$data[2]}};
      }
      $_= sprintf("%d:%02x%02x%02x%02x%02x%02x", split(/\./,$data[1]));
      @_= split(/:/,$_);
      $id= $_[0];
      $mac = $_[1];
      $portNames{$mac}=$vlanNames{$id};
    }
    #do the getnext at the end, because if we're on the last, the next
    #one is junk to all the processing instructions...
    $sess->getnext($field);
  } while ( $data[0] =~ /^(policyVlan)|(policyMacRuleVlanId)/);
  #Next line is to eliminate some incorrect warnings...
  if (%Link) { } if (%portNames){} if (%duplex){} if (%auto){}
  my $switch = (defined $Dev{$i} ? $Dev{$i} : "--unknown--");
  print "Port Configuration, Testbed Switch $switch ($i)\n";
  print "Port Interface\tVLAN\tEnabled\tLink\tAutoNeg\tSpeed\t\tDuplex\n";
  print "----------------------------------------------------------------";
  print "------------\n";
  foreach my $id ( sort num keys (%Able) ) {
    my $vlan;
    my $switchport = join(":",$i,$id);
    my $ifname = $Ports{$switchport};
    if (! defined ($ifname) ) {
      $ifname = "\t";
    }
    if (! defined ($auto{$id}) ) {
      $auto{$id} = "\t";
    }
    if (! defined ($speed{$id}) ) {
      $speed{$id} = "        ";
    }
    if (! defined ($duplex{$id}) ) {
      $duplex{$id} = "\t";
    }
    if ($id < 100) { print " "; }
    if ($id < 10) { print " "; }
    print $id,"  ";
    print $ifname,"\t";
    if (defined ($Interfaces{$ifname})) {
      $vlan = $portNames{$Interfaces{$ifname}};
    }
    if ( defined $vlan) {
      print $vlan;
    }
    print "\t";
    print $Able{$id},"\t",$Link{$id},"\t";
    print $auto{$id},"\t",$speed{$id},"\t";
    if (length ($speed{$id}) < 8 ) {
      print "\t";
    }
    print $duplex{$id},"\n";
  }
}

#
# SUB GetStats ----------------------
#

sub GetStats {
  my $sess = shift;
  $sess = $$sess;		# unreference it...
  my $MinPorts = 1;
  my $MaxPorts = 24;
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
  $sess->getnext($ifTable);
  do {
    @data = @{$ifTable};
    print "$data[0]\t$data[1]\t$data[2]\n" if $debug;
    if (($data[1] >= $MinPorts) && ($data[1] <= $MaxPorts)) {
      if ($data[0]=~/InOctets/) {
	$inOctets{$data[1]}  = $data[2];
      } elsif ($data[0]=~/InUcast/) {
	$inUcast{$data[1]}   = $data[2];
      } elsif ($data[0]=~/InNUcast/) {
	$inNUcast{$data[1]}  = $data[2];
      } elsif ($data[0]=~/InDiscard/) {
	$inDiscard{$data[1]} = $data[2];
      } elsif ($data[0]=~/InErrors/) {
	$inErr{$data[1]}     = $data[2];
      } elsif ($data[0]=~/InUnknownP/) {
	$inUnkProt{$data[1]} = $data[2];
      } elsif ($data[0]=~/OutOctets/) {
	$outOctets{$data[1]} = $data[2];
      } elsif ($data[0]=~/OutUcast/) {
	$outUcast{$data[1]}  = $data[2];
      } elsif ($data[0]=~/OutNUcast/) {
	$outNUcast{$data[1]} = $data[2];
      } elsif ($data[0]=~/OutDiscard/) {
	$outDiscard{$data[1]}= $data[2];
      } elsif ($data[0]=~/OutErrors/) {
	$outErr{$data[1]}    = $data[2];
      } elsif ($data[0]=~/OutQLen/) {
	$outQLen{$data[1]}   = $data[2];
      }
    }
    $sess->getnext($ifTable);
  } while ( $data[0] =~ /^i[f](In|Out)/) ;
  my $switch = (defined $Dev{$i} ? $Dev{$i} : "--unknown--");
  print "Port Statistics, Testbed Switch $switch ($i)\n";
  print "\t\t\t     InUcast   InNUcast  In        In      Unknown\t".
    "\t  OutUcast  OutNUcast Out       Out      Out Queue\n";
  print "Port Interface\tIn Octets    Packets   Packets   Discards  Errors".
    "  Proto.    Out Octets   Packets   Packets   Discards  Errors   ".
      "Length\n";
  print "------------------------------------------------------------------";
  print "------------------------------------------------------------------";
  print "------\n";
  foreach my $port ( sort num keys (%inOctets) ) {
    my $vlan;
    my $switchport = join(":",$i,$port);
    my $ifname = $Ports{$switchport};
    if (! defined ($ifname) ) {
      $ifname = "         ";
    }
    if ($port < 10) { print " "; }
    print " ",$port,"  ";
    print $ifname,"\t";
    my $n=0;
    my $str="";
    print $str=$inOctets{$port};
    foreach $n (length($str)..12) { print " "; }
    print $str=$inUcast{$port};
    foreach $n (length($str)..9) { print " "; }
    print $str=$inNUcast{$port}; 
    foreach $n (length($str)..9) { print " "; }
    print $str=$inDiscard{$port};
    foreach $n (length($str)..9) { print " "; }
    print $str=$inErr{$port}; 
    foreach $n (length($str)..7) { print " "; }
    print $str=$inUnkProt{$port};
    foreach $n (length($str)..9) { print " "; }
    print $str=$outOctets{$port}; 
    foreach $n (length($str)..12) { print " "; }
    print $str=$outUcast{$port};
    foreach $n (length($str)..9) { print " "; }
    print $str=$outNUcast{$port}; 
    foreach $n (length($str)..9) { print " "; }
    print $str=$outDiscard{$port};
    foreach $n (length($str)..9) { print " "; }
    print $str=$outErr{$port}; 
    foreach $n (length($str)..9) { print " "; }
    print $str=$outQLen{$port},"\n";
  }
}

# End with true
1;

