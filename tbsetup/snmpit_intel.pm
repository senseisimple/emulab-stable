#!/usr/bin/perl -w
#
# snmpit module for Intel EtherExpress 510T switches
#

# Special Note:
# Because this code is not urgently needed, and it is unknown if it ever
# will be needed again, it has not been debugged since moving it into 
# this module. So don't count on it. It will need to be debugged before
# actually using it for anything useful.

package snmpit_intel;
use strict;

$| = 1;				# Turn off line buffering on output

use English;
use SNMP;
use snmpit_lib;

#
# These are the commands that can be passed to the portControl function
# below
#
my %cmdOIDs = 
(
    "enable" => [".1.3.6.1.2.1.2.2.1.7","up"],
    "disable"=> [".1.3.6.1.2.1.2.2.1.7","down"],
    "100mbit"=> [".1.3.6.1.4.1.343.6.10.2.4.1.10.1.1","speed100Mbit"],
    "10mbit" => [".1.3.6.1.4.1.343.6.10.2.4.1.10.1.1","speed10Mbit"],
    "full"   => [".1.3.6.1.4.1.343.6.10.2.4.1.11.1.1","full"],
    "half"   => [".1.3.6.1.4.1.343.6.10.2.4.1.11.1.1","half"],
    "auto"   => [".1.3.6.1.4.1.343.6.10.2.4.1.12.1.1","auto"]
);


#
# Creates a new object.
#
# usage: new($classname,$devicename,$debuglevel)
#        returns a new object, blessed into the snmpit_intel class.
#
sub new {

    # The next two lines are some voodoo taken from perltoot(1)
    my $proto = shift;
    my $class = ref($proto) || $proto;

    my $name = shift;
    my $debugLevel = shift;

    #
    # Create the actual object
    #
    my $self = {};

    #
    # Set the defaults for this object
    # 
    if (defined($debugLevel)) {
	$self->{DEBUG} = $debugLevel;
    } else {
	$self->{DEBUG} = 0;
    }
    $self->{BLOCK} = 1;
    $self->{NAME} = $name;

    if ($self->{DEBUG}) {
	print "snmpit_cisco module initializing... debug level $self->{DEBUG}\n"
	;   
    }

    #
    # Set up SNMP module variables, and connect to the device
    #
    $SNMP::debugging = ($self->{DEBUG} - 2) if $self->{DEBUG} > 2;
    my $mibpath = '/usr/local/share/snmp/mibs';
    &SNMP::addMibDirs($mibpath);
    &SNMP::addMibFiles("$mibpath/INT_GEN.MIB", 
		       "$mibpath/INT_S500.MIB",
		       "$mibpath/INT_VLAN.MIB");
    $SNMP::save_descriptions = 1; # must be set prior to mib initialization
    SNMP::initMib();		# parses default list of Mib modules 
    $SNMP::use_enums = 1;		#use enum values instead of only ints

    warn ("Opening SNMP session to $self->{NAME}...") if ($self->{DEBUG});

    # XXX: SNMP version?
    $self->{SESS} = new SNMP::Session(DestHost => $self->{NAME});

    if (!$self->{SESS}) {
	#
	# Bomb out if the session could not be established
	#
	warn "ERROR: Unable to connect via SNMP to $self->{NAME}\n";
	return undef;
    }

    bless($self,$class);
    return $self;
}

#
# Set a variable associated with a port. The commands to execute are given
# in the cmdOIs hash above
#
# usage: portControl($self, $command, @ports)
#        returns 0 on success.
#        returns number of failed ports on failure.
#        returns -1 if the operation is unsupported
#
sub portControl($$@) {
    my $self = shift;

    my $cmd = shift;
    my @ports = @_;

    $self->debug("portControl: $cmd -> (@ports)\n");

    # XXX: portnum
    my @p = map { portnum($_) } @ports;

    #
    # Find the command in the %cmdOIDs hash (defined at the top of this file)
    #
    my @oid = @{$cmdOIDs{$cmd}};
    if (defined @oid) {
	return &UpdateField(\$oid[0],\$oid[1],\@p);
    } else {
	#
	# Command not supported
	#
	print STDERR "Unsupported port control command '$cmd' ignored.\n";
	return -1;
    }
}

sub vlanLock($) {
    my $self = shift;

    my $TokenOwner = '.1.3.6.1.4.1.343.6.11.4.5';
    my $TokenReq = '.1.3.6.1.4.1.343.6.11.4.6';
    my $TokenReqResult = '.1.3.6.1.4.1.343.6.11.4.7';
    my $TokenRelease = '.1.3.6.1.4.1.343.6.11.4.8';
    my $TokenReleaseResult = '.1.3.6.1.4.1.343.6.11.4.9';

    #
    # Some magic needed by the Intel switch
    #
    my $Num = pack("C*",0,0,0,0,1,1);

    #
    # Try max_tries times before we give up, in case some other process just
    # has it locked.
    #
    my $tries = 1;
    my $max_tries = 20;
    while ($tries <= $max_tries) {

	#
	# Attempt to grab the edit buffer
	#
	$self->{SESS}->set([[$TokenReq,0,$Num,"OCTETSTR"]]);
	my $RetVal = $self->{SESS}->get([[$TokenReqResult,0]]);
	while ($RetVal eq "notReady") {
	    $RetVal = $self->{SESS}->get([[$TokenReqResult,0]]);
	    $self->debug("*VLAN Token Claim Result is $RetVal\n");
	    select (undef, undef, undef, .25); #wait 1/4 second
	}

	$self->debug("VLAN Token Claim Result is $RetVal\n");

	if ($RetVal ne 'success') {
	    #
	    # Only print this message every five tries
	    #
	    if (!($tries % 5)) {
		print STDERR "VLAN edit buffer request failed - " .
		"try $tries of $max_tries.\n";
	    }
	} else {
	    last;
	}
	$tries++;

	sleep(1);
    }

    if ($tries > $max_tries) {
	#
	# Admit defeat and exit
	#
	print STDERR "ERROR: Failed to obtain VLAN edit buffer lock\n";
	return 0;
    } else {
	return 1;
    }

}

#
# Release a lock on the VLAN edit buffer. As part of releasing, applies the
# VLAN edit buffer.
#
# usage: vlanUnlock($self)
#        
sub vlanUnlock($;$) {
    my $self = shift;

    my $TokenRelease = '.1.3.6.1.4.1.343.6.11.4.8';
    my $TokenReleaseResult = '.1.3.6.1.4.1.343.6.11.4.9';
    my $TokenConfirmState = '.1.3.6.1.4.1.343.6.11.1.18';

    my $save = ($self->{CONFIRM} ? "saveWithConfirmOption" : "save");

    $self->debug("Releasing Token with $save command\n");
    $self->{SESS}->set([[$TokenRelease,0,$save,"INTEGER"]]);
    my $RetVal = $self->{SESS}->get([[$TokenReleaseResult,0]]);
    $self->debug("VLAN Configuration Save Result is $RetVal\n");

    while ($RetVal eq "notReady") {
	$RetVal = $self->{SESS}->get([[$TokenReleaseResult,0]]);
	$self->debug("VLAN Configuration Save Result is $RetVal\n");
	select (undef, undef, undef, .25); #wait 1/4 second
    }

    if ($self->{CONFIRM}) {
	$RetVal = $self->{SESS}->get([[$TokenConfirmState,0]]);
	$self->debug("VLAN Configuration Confirm Result is $RetVal\n");

	while ($RetVal eq "notReady") {
	    sleep(2);
	    $RetVal = $self->{SESS}->get([[$TokenConfirmState,0]]);
	    $self->debug("VLAN Configuration Confirm Result is $RetVal\n");
	}

	if ($RetVal eq "ready") {
	    $RetVal = $self->{SESS}->set([[$TokenConfirmState,0,"confirm","INTEGER"]]);
	    $self->debug("VLAN Configuration Confirm Result is $RetVal\n");
	} # XXX: Should there be an 'else'?

	while (!($RetVal =~ /Conf/i)) {
	    $RetVal = $self->{SESS}->get([[$TokenConfirmState,0]]);
	    $self->debug("VLAN Configuration Confirm Result is $RetVal\n");
	}

	if ($RetVal ne "confirmedNewConf") {
	    die("VLAN Reconfiguration Failed. No changes saved.\n");
	}

    } else {
	if ($RetVal ne 'success') {
	    die("VLAN Reconfiguration Failed: $RetVal. No changes saved.\n");
	}
    }
}

# XXX: Changeme!
#sub setupVlan {
#  # This is to be called ONLY after vlanLock has been called successfully!
#  my $sess = shift;
#  $sess = $$sess;		# unreference it...
#  my $name = shift;
#  my @vlan = @_;
#  foreach my $mac (@vlan) {
#    my $node = $Interfaces{$mac};
#    if (! NodeCheck($node)) { 
#      print STDERR "You are not authorized to control $node.\n";
#      return 1;
#    }
#  }
#
#  my $NextVLANId = '.1.3.6.1.4.1.343.6.11.1.6';
#  my $Vlan = $sess->get([[$NextVLANId,0]]);
#  my $CreateOID = ".1.3.6.1.4.1.343.6.11.1.9.1.3";
#  my $RetVal = "Undef.";
#  if ( !$name 
#       #Temporary fix: if its a lX-Y name from assign.tcl, ignore it
#       || ($name =~ /^l(\d)+-(\d)+/)
#       #End of Temp fix
#     ) {
#    $name = $Vlan;
#  }
#  print "  Creating VLAN $name as VLAN #$Vlan: @vlan ... ";
#  $RetVal = $sess->set([[$CreateOID,$Vlan,$name,"OCTETSTR"]]);
#  print "",($RetVal? "Succeeded":"Failed"),".\n";
#  if (! defined ($RetVal) ) {
#    &vlanUnlock(*sess,$verbose,1);
#    die("VLAN name \"$name\" not unique.\n");
#  }
#  my @x;
#  my $n=0;
#  while (@vlan != 0 && $n < @vlan) {
#    my $i=0;
#    while ($i < 6 ) {
#      $x[$i] = hex ("0x".substr($vlan[$n],2*$i,2) );
#      $i++;
#    }
#    my $MacObjOID = ".1.3.6.1.4.1.343.6.11.1.10.1.3.$Vlan." . 
#      "$x[0].$x[1].$x[2].$x[3].$x[4].$x[5]";
#    print "    Adding MAC Address $vlan[$n] ".
#      "($Vlan.$x[0].$x[1].$x[2].$x[3].$x[4].$x[5])... ";
#    $RetVal = $sess->set([[$MacObjOID,0,$vlan[$n],"OCTETSTR"]]);
#    print "",($RetVal? "Succeeded":"Failed"), ".\n";
#    $n++;
#  }
#}

#
# Remove the given VLAN from this switch. This presupposes that all of its
# ports have already been removed with removePortsFromVlan(). The VLAN is
# given as a VLAN identifier from the database.
#
# usage: removeVlan(self,int vlan)
#        returns 1 on success
#        returns 0 on failure
#
#
sub removeVlan($$) {
    my $self = shift;
    my $vlan_id = shift;

    #
    # Find the real VLAN number from the passed VLAN ID
    #
    my $vlan_number = $self->findVlan($vlan_id);
    if (!defined($vlan_number)) {
	print STDERR "ERROR: VLAN with identifier $vlan_id does not exist\n";
	return 0;
    }
    $self->debug("Found VLAN with ID $vlan_id: $vlan_number\n");

    #
    # Need to lock the VLAN edit buffer
    #
    if (!$self->vlanLock()) {
	return 0;
    }

    #
    # Perform the actual removal
    #
    my $DeleteOID = ".1.3.6.1.4.1.343.6.11.1.9.1.4";
    my $RetVal = "Undef.";

    print "  Removing VLAN #$vlan_number ... ";
    $RetVal = $self->{SESS}->set([[$DeleteOID,$vlan_number,"delete","INTEGER"]]);
    print "",($RetVal? "Succeeded":"Failed"),".\n";

    #
    # Unlock whether successful or not
    #
    $self->vlanUnlock();

    if (! defined $RetVal) {
	print STDERR "VLAN #$vlan_number does not exist on this switch.\n";
	return 0;
    } else {
	$self->vlanUnlock();
	return 1;
    }

}

#
# XXX: Major cleanup
#
sub UpdateField($$$@) {
    my $self = shift;
    my ($OID,$val,@ports)= @_;

    my $Status = 0;
    my $err = 0;

    $self->debug("UpdateField: '@_'\n");

    foreach my $port (@ports) {
	$self->debug("Checking port $port for $val...");
	$Status = $self->{SESS}->get([[$OID,$port]]);
	if (!defined $Status) {
	    print STDERR "Port $port, change to $val: No answer from device\n";
	    return -1;			# Return error
	} else {
	    $self->debug("Okay.\n");
	    $self->debug("Port $port was $Status\n");
	    if ($Status ne $val) {
		$self->debug("Setting $port to $val...");
		# The empty sub {} is there to force it into async mode
		$self->{SESS}->set([[$OID,$port,$val,"INTEGER"]],sub {});
		if ($self->{BLOCK}) {
		    while ($Status ne $val) { 
			$Status = $self->{SESS}->get([[$OID,$port]]);
			$self->debug("Value for $port was ",$Status,"\n");
			# XXX: Timeout
		    }
		    print "Okay.\n";
		} else {
		    print "\n";
		}
	    }
	}

    }
    if ( (!$self->{BLOCK}) && !$self->{CONFIRM} ) {
	my $loops = 0;
	my $max_loops = 10;
	my %notdone = ();
	my @done = ();
	foreach my $port (@ports) {
	    $Status = $self->{SESS}->get([[$OID,$port]]);
	    $self->debug("Value for $port was $Status\n");
	    if ($Status ne $val) {
		$notdone{$port} =1;
	    }
	}
	while ( %notdone && $loops < $max_loops ) {
	    if ($loops > 5) {
		sleep($loops -5);
	    }
	    foreach my $port (sort num keys(%notdone)) {
		$Status = $self->{SESS}->get([[$OID,$port]]);
		$self->debug("Value for $port was ",$Status,"\n");
		if ($Status eq $val) {
		    push(@done,$port);
		}
	    }
	    foreach my $i (@done) {
		delete $notdone{$i};
	    }
	    $loops++;
	}
	if ($loops == $max_loops) {
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

#
# List all VLANs on the device
#
# usage: listVlans($self)
# see snmpit_cisco_stack.pm for a description of return value format
#
sub listVlans {
    my $self = shift;

	# XXX: If Intels support snmp v2c, use bulkget for this part
	my %Names = ();
	my %Members = ();
	my @data = ();
	my $mac = "";
	my $node= "";
	my @vlan = ();
	my $field = ["vlanPolicyVlanTable",0];
	#do one to get the first field...
	$self->{SESS}->getnext($field);
	do {
		@data = @{$field};
		$self->debug("$data[0]\t$data[1]\t$data[2]\n");
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

			# XXX: FIX!
			#if (defined ( $Interfaces{$mac} ) ) {
			#	$node = $Interfaces{$mac};
			#} else {
				$node = $mac;
			#}

			push(@vlan, $node);
			$Members{$data[2]} = \@vlan;
		}
		#do the getnext at the end, because if we're on the last, the next
		#one is junk to all the processing instructions...
		$self->{SESS}->getnext($field);
	} while ( $data[0] =~ /^(policyVlan)|(policyMacRuleVlanId)/) ;

	#
	# Build a list from the name and membership lists
	#
	my @list = ();

	foreach my $vlan_id ( sort {$a <=> $b} keys %Names ) {
		push @list, [$Names{$vlan_id},$vlan_id,$Members{$vlan_id}];
	}

	$self->debug(join("\n",@list)."\n",2);
	return @list;
}

#   
# Prints out a debugging message, but only if debugging is on. If a level is
# given, the debuglevel must be >= that level for the message to print. If
# the level is omitted, 1 is assumed
#   
# Usage: debug($self, $message, $level)
#   
sub debug($$:$) {
    my $self = shift;
    my $string = shift;
    my $debuglevel = shift;
    if (!(defined $debuglevel)) {
	$debuglevel = 1;
    }
    if ($self->{DEBUG} >= $debuglevel) {
	print STDERR $string;
    }
}


1;

__END__
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

