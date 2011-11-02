#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005-2011 University of Utah and the Flux Group.
# All rights reserved.
#
use Getopt::Std;
use English;

my $datafile = "fw-rules";

my $optlist = "eMIf:";
my $domysql = 0;
my $doipfw = 1;
my $expand = 0;
my @lines;

sub usage()
{
    print "Usage: genconfig [-MI] config ...\n".
	"  -e      expand EMULAB_ variables\n".
	"  -f file specify the input rules file\n".
	"  -M      generate mysql commands\n".
	"  -I      generate IPFW commands\n".
	"\n".
	" Valid configs are: open, closed, basic, elabinelab\n";
    exit(1);
}

my %fwvars;

sub getfwvars()
{
    # XXX for Utah Emulab as of 11/11
    $fwvars{EMULAB_GWIP} = "155.98.36.1";
    $fwvars{EMULAB_GWMAC} = "00:d0:bc:f4:14:f8";
    $fwvars{EMULAB_NS} = "155.98.32.70";
    $fwvars{EMULAB_CNET} = "155.98.36.0/22";
    $fwvars{EMULAB_BOSSES} = "boss,subboss";
    $fwvars{EMULAB_MCADDR} = "234.0.0.0/8";
    $fwvars{EMULAB_MCPORT} = "1025-65535";
}

sub expandfwvars($)
{
    my ($rule) = @_;

    getfwvars() if (!%fwvars);

    if ($rule =~ /EMULAB_\w+/) {
	foreach my $key (keys %fwvars) {
	    $rule =~ s/$key/$fwvars{$key}/g
		if (defined($fwvars{$key}));
	}
	if ($rule =~ /EMULAB_\w+/) {
	    warn("*** WARNING: Unexpanded firewall variable in: \n".
		 "    $rule\n");
	}
    }
    return $rule;
}

sub doconfig($)
{
    my ($config) = @_;
    my $ruleno = 1;
    my ($type, $style, $enabled);

    if ($doipfw) {
	print "# $config\n";
	print "ipfw -q flush\n";
    }
    if ($domysql) {
	$type = "ipfw2-vlan";
	$style = lc($config);
	# XXX
	$style = "emulab" if ($style eq "elabinelab");
	$enabled = 1;

	print "DELETE FROM `default_firewall_rules` WHERE ".
	    "type='$type' AND style='$style';\n";
    }

    foreach my $line (@lines) {
	next if ($line !~ /#.*$config/);
	next if ($line =~ /^#/);
	if ($line =~ /#\s*(\d+):.*/) {
	    $ruleno = $1;
	} else {
	    $ruleno++;
	}
	($rule = $line) =~ s/\s*#.*//;
	chomp($rule);
	$rule = expandfwvars($rule) if ($expand);
	if ($doipfw) {
	    print "ipfw add $ruleno $rule\n";
	}
	if ($domysql) {
	    print "INSERT INTO `default_firewall_rules` VALUES (".
		"'$type','$style',$enabled,$ruleno,'$rule');\n";
	}
    }

    print "\n";
}

%options = ();
if (! getopts($optlist, \%options)) {
    usage();
}
if (defined($options{"M"})) {
    $domysql = 1;
    $doipfw = 0;
}
if (defined($options{"I"})) {
    $doipfw = 1;
    $domysql = 0;
}
if (defined($options{"e"})) {
    $expand = 1;
}
if (defined($options{"f"})) {
    $datafile = $options{"f"};
}

if (@ARGV == 0) {
    usage();
}
@lines = `cat $datafile`;
foreach my $config (@ARGV) {
    $config = uc($config);
    doconfig($config);
}
exit(0);
