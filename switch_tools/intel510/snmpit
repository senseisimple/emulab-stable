#!/usr/local/bin/perl -w
#########################################################################
# SnmpIt! - An SNMP Tool for the Intel 510T Switch                       #
#                                                                       #
# Run with -h option to see command line syntax/options.                #
#                                                                       #
#      Author: Mac Newbold, Flux Research Group, University of Utah     #
# Last Change: March 1999                                               #
#     Version: 0.1b                                                     #
#                                                                       #
#########################################################################

#
# INIT SECTION =========================================================
#
use lib '/n/moab/x/newbold/ucd-snmp-4.1.1/perl/SNMP/blib/lib';
use lib '/n/moab/x/newbold/ucd-snmp-4.1.1/perl/SNMP/blib/arch/auto/SNMP';
use SNMP;
#$SNMP::debugging = 1;

my $S = "";  #Switch IP addr.
my @p = ();  #Port numbers
my $D = 0;   #Disable (bool)
my $E = 0;   #Enable (bool)
my $a = "";  #Auto Negotiation (enable/disable) (two-way switch)
my $d = "";  #Duplex (half/full)
my $s = 0;   #Speed (10/100)
my $C = "";  #Create VLAN - name
my @v = ();  #VLAN Members - list of MAC Addresses
my $B = 0;   #Block (bool) (two-way switch)
my $V = 1;   #Verify Only (bool) (two-way switch)
my $L = 0;   #Loud/Verbose (bool) !Must default quiet; switch only turns on!

&ParseArgs(*ARGV,*S,*p,*D,*E,*a,*d,*s,*C,*v,*B,*V,*L);
#print "Returned: S=$S p=@p D=$D E=$E a=$a d=$d s=$s C=$C v=@v B=$B\n";

#print "Loading MIBs...\n";
&SNMP::addMibDirs('/usr/local/share/snmp/mibs');
&SNMP::addMibFiles('/usr/local/share/snmp/mibs/RFC1155-SMI.txt',
		   '/usr/local/share/snmp/mibs/RFC1213-MIB.txt',
		   '/usr/local/share/snmp/mibs/BRIDGE-MIB.txt', 
		   '/usr/local/share/snmp/mibs/EtherLike-MIB.txt',
		   '/usr/local/share/snmp/mibs/RMON-MIB.txt',
		   '/usr/local/share/snmp/mibs/INTEL-GEN-MIB.txt', 
		   '/usr/local/share/snmp/mibs/INTEL-S500-MIB.txt',
		   '/usr/local/share/snmp/mibs/INTEL-VLAN-MIB.txt',
		   '/usr/local/share/snmp/mibs/DVMRP-MIB.txt', 
		   '/usr/local/share/snmp/mibs/INTEL-L3LINK-MIB.txt',
		   '/usr/local/share/snmp/mibs/IGMP-MIB.txt',
		   '/usr/local/share/snmp/mibs/INTEL-IPF-MIB.txt', 
		   '/usr/local/share/snmp/mibs/INTEL-IPROUTER-MIB.txt',
		   '/usr/local/share/snmp/mibs/INTEL-IGMP-PRUNING-MIB.txt', 
		   '/usr/local/share/snmp/mibs/INTEL-IPX-MIB.txt');
$SNMP::save_descriptions = 1; # must be set prior to mib initialization
 SNMP::initMib(); # parses default list of Mib modules from default dirs

#
# MAIN SECTION =========================================================
#

$SNMP::use_enums = 1; #use enum values instead of only ints
my $sess = new SNMP::Session(DestHost => $S);

if ($D || $E) {
    my $Admin = ".1.3.6.1.2.1.2.2.1.7";
    my $Status = "";
    $Status = "down" if ($D);
    $Status = "up" if ($E);
    if (! &Update(*sess,$Admin,*p,$Status,*B,*V,*L)) {
	print STDERR "Port ",($D?"disable":"enable")," failed.\n";
    }    
}

if ($s) {
    my $Spd = ".1.3.6.1.4.1.343.6.10.2.4.1.10.1.1";
    if (! &Update(*sess,$Spd,*p,$s,*B,*V,*L)) {
	print STDERR "Port ",($D?"disable":"enable")," failed.\n";
    }    
}

if ($d) {
    my $Dup = ".1.3.6.1.4.1.343.6.10.2.4.1.11.1.1";
    if (! &Update(*sess,$Dup,*p,$d,*B,*V,*L)) {
	print STDERR "Port ",($D?"disable":"enable")," failed.\n";
    }    
}

if ($a) {
    my $Auto = ".1.3.6.1.4.1.343.6.10.2.4.1.12.1.1";
    my $aOp = ( $a=~/en/ ? "auto" : "manual");
    if (! &Update(*sess,$Auto,*p,$a,*B,*V,*L)) {
	print STDERR "Port ",($D?"disable":"enable")," failed.\n";
    }    
}

#
# END OF MAIN  =================================================
#



#
# SUBROUTINE SECTION =================================================
#

#
# SUB PARSEARGS----------------------
#
sub ParseArgs {
    local(*CMDS,*S,*p,*D,*E,*a,*d,*s,*C,*v,*B,*V,*L) = @_;
    my $help=0; 
    if (@CMDS < 3) {$help = 2; }
    while (@CMDS != 0 && $CMDS[0] =~ /^(-|\+)/) {
	$_ = shift(@CMDS);
	#print "Item=$_\n";
	#print "S=$S p=@p D=$D E=$E a=$a d=$d s=$s C=$C v=@v B=$B V=$V\n";
	if (/^-S(.*)/) {$S = ($1 ? $1 : shift(@CMDS));}
	elsif (/^-p(.*)/) {
	    push(@p, ($1 ? $1 : shift(@CMDS)));
	    while ( @CMDS>0 && ! ($CMDS[0] =~ /^(-|\+)(.*)/ ) ) { 
		push(@p, shift(@CMDS));
	    }
	}
	elsif (/^-R(.*)/) {
	    my $Range ="";
	    $Range .= ($1 ? $1 : shift(@CMDS));
	    if ( $Range =~ /^(\d*)\.\.(\d*)$/ ) {
		push(@p, $1..$2);
	    } else {
		die("Invalid Range: $Range\n");
	    }	   
	}
	elsif (/^-h(.*)/) {$help = 1;}
	elsif (/^-L(.*)/) {$L = 1;}
	elsif (/^-D(.*)/) {$D = 1;}
	elsif (/^-E(.*)/) {$E = 1;}
	elsif (/^-a(.*)/) {$a="disable";}
	elsif (/^\+a(.*)/) {$a="enable";}
	elsif (/^-d(.*)/) {$d = ($1 ? $1 : shift(@CMDS));}
	elsif (/^-s(.*)/) {$s = ($1 ? $1 : shift(@CMDS));}
	elsif (/^-C(.*)/) {$C = ($1 ? $1 : shift(@CMDS));}
	elsif (/^-v(.*)/) {
	    push(@v, ($1 ? $1 : shift(@CMDS)));
	    while ( @CMDS>0 && ! ($CMDS[0] =~ /^(-|\+)(.*)/ ) ) { 
		push(@v, shift(@CMDS));
	    }
	}
	elsif (/^-B(.*)/) {$B = 0;}
	elsif (/^\+B(.*)/) {$B = 1;}
	elsif (/^-V(.*)/) {$V = 0;}
	elsif (/^\+V(.*)/) {$V = 1;}
	else {die("Unknown Option: $_\n");}
    }
    #print "\nDone:\nS=$S p=@p D=$D E=$E a=$a d=$d s=$s C=$C v=@v B=$B V=$V\n";
    if ($help) {
	print
	    "SnmpIt! - An SNMP Tool for the Intel 510T Switch\n",
	    "Syntax:\n",
	    "$0 [-h] [-L] -S<switch> [(+|-)B] [(+|-)V]\n",
	    "\t[-p<port> <port> <port>...] [-R <x>..<y>]\n",
	    "\t[-D|-E] [[+|-]a] [-s<speed>] [-d<duplex>]\n", 
	    "\t[-C<vlan name> -v<vlan memeber MAC>]\n";
    }
    if ($help == 1) {
	print
	    "\n",
	    "  -h    Display this help message\n",
	    "  -L    Loud/Verbose mode (now ",($L?"on":"off"),")\n",
	    "  -S    Switch name or IP address\n",
	    "  +B/-B Blocking mode (now ",($B?"on":"off"),")\n",
	    "  +V/-V Verify (when done with list; now ",($V?"on":"off"),")\n",
	    "  -p    List of port numbers on which to perform operations\n",
	    "  -R    Range of port numbers\n",
	    "  -D    Disable port(s)\n",
	    "  -E    Enable port(s)\n",
	    "  +a/-a Enable/Disable Port Auto-Negotiation of speed/duplex\n",
	    "  -s    Port Speed (10 or 100 Mbits)\n",
	    "  -d    Port Duplex (half or full)\n",
	    "  -C    Create VLAN\n",
	    "  -v    Add member to VLAN\n";
    }
    #If help only, end here...
    die("\b\n") if ($help);
    #Now die on any combinations that don't make sense...
    die("What shall I do?\n")
	if (! ($D || $E || $a || $s || $d || $C) );
    if ($S =~ /Alpha/i) { $S = "155.99.214.170"; }
    if ($S =~ /Beta/i)  { $S = "155.99.214.171"; }
    die("Unknown Switch $S: Possible completions are\n",
	"Alpha/155.99.214.170\n Beta/155.99.214.171\n") 
	if (!(($S=~/155\.99\.214\.(.*)/) && ($1=~ /^170$|^171$/)));
    die("Can't enable and disable at the same time.\n") if ($D && $E);
    die("Can't use auto with duplex or speed.\n") if (($a=~/en/)&&($d || $s));
    my $n=0;
    while(@p != 0 && $n < @p) {
	die("Invalid port ",$p[$n],": Must be 1-24\n") 
	    if ($p[$n] =~ /\D/ || $p[$n]>24 || $p[$n]<1);
	$n++;
    }
    die("Which port should I reconfigure?\n") if (!@p &&($D||$E||$d||$s||$a));
    die("Can't create VLAN without name.\n") if ( @v && !$C );
    die("Invalid duplex $d: Must be full or half\n") 
	if(!($d=~/^full$|^half$|^$/));
    die("Invalid speed $s: Must be 10 or 100\n") if (!($s=~/^10$|^100$|^0$/));
    if ($s eq "10" ) { $s="speed10Mbit" ;}
    if ($s eq "100") { $s="speed100Mbit";}
    $n=0;
    while(@v != 0 && $n < @v) {
	die("Invalid MAC Address ",$v[$n],". Must be 6 byte Hex value.\n")
	    if (! ( ($v[$n]=~ /^([a-f]|\d)*$/) && (length ($v[$n]) == 12)));
	$n++;
    }
}

#
# SUB UPDATE----------------------
#

sub Update {    
    local(*sess,$OID,*p,$val,*B,*V,*L)= @_;
    #print "$sess\nOID:$OID\np=@p\nval=$val\n$B $V $L\n";
    my $Status = 0;
    foreach $port (@p) {
	$Status = $sess->get([[$OID,$port]]);
	print "Got value for $port: $Status\n" if ($L);
	if ($Status ne $val) {
	    print "Setting value for port $port to $val\n" if ($L);
	    #The empty sub {} is there to force it into async mode
	    $sess->set([[$OID,$port,$val,"INTEGER"]],sub {});
	    if ($B) {
		while ($Status ne $val) { 
		    $Status=$sess->get([[$OID,$port]]);
		    print "Got value for:$port ",$Status,"\n" if ($L);
		}
	    }
	}
    }
    if ( (!$B) && $V ) {
	my $loops=0;
	my $max_loops=10;
	my %notdone=();
	my @done=();
	foreach $port (@p) {
	    $Status=$sess->get([[$OID,$port]]);
	    print "Got value for:$port ",$Status,"\n" if ($L);
	    if ($Status ne $val) {
		$notdone{$port}=1;
	    }
	}
	while ( %notdone && $loops < $max_loops ) {
	    foreach $port (sort keys(%notdone)) {
		$Status=$sess->get([[$OID,$port]]);
		print "Got value for:$port ",$Status,"\n" if ($L);
		if ($Status eq $val) {
		    push(@done,$port);
		}
	    }
	    foreach $i (@done) { delete $notdone{$i}};
	    $loops++;
	}
	if ($loops==$max_loops) {
	    foreach $port (sort keys(%notdone)) {
		print STDERR "Port $port Change not verified!\n";
	    }
	    return(0); #Return False!
	}
    }
    1;
}

#
# SUB MAKEVLAN----------------------
#

sub MakeVLAN {
    local(*sess,*C,*v,*B,*V,*L) = @_;

#mib2ext.vlan.vlanPolicy.vlanPolicyNextVlanId = 6.11.1.6
#mib2ext.vlan.vlanPolicy.vlanPolicyChangeOperation = 6.11.1.8 (create/rename)
#mib2ext.vlan.vlanPolicy.vlanPolicyVlanTable = 6.11.1.9
#mib2ext.vlan.vlanPolicy.vlanPolicyMacRuleTable = 6.11.1.10
#mib2ext.vlan.vlanPolicy.vlanPolicyIpRuleTable = 6.11.1.12
#mib2ext.vlan.vlanPolicy.vlanPolicyIpNetRuleTable = 6.11.1.13
#mib2ext.vlan.vlanPolicy.vlanPolicyPortRuleTable = 6.11.1.14
#mib2ext.vlan.vlanPolicy.vlanPolicyRevert2Default = 6.11.1.21
#mib2ext.vlan.vlanLearned.vlanLearnedMacTable = 6.11.2.9
#mib2ext.vlan.vlanEditToken. = 6.11.4.
#mib2ext.vlan.vlanEditToken. = 6.11.4.
#mib2ext.vlan.vlanEditToken. = 6.11.4.
#mib2ext.vlan.vlanEditToken. = 6.11.4.
#mib2ext.vlan.vlanEditToken. = 6.11.4.
#mib2ext.vlan.vlanEditToken. = 6.11.4.



}

#
# SUB ----------------------
#

#
# END OF SUBROUTINE SECTION =========================================
#
#
# END OF PROGRAM  ===================================================
#
