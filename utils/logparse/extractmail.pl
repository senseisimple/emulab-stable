#!/usr/bin/perl -w
#
# Extract testbed mail of interest.  Takes one or more mboxs of interest
# and creates a new file with only the messages of interest.  Duplicates
# are ignored.  Note that the resulting file is NOT necessarily in date order.
#
# Works for 2000-2005 mail.
#
# cat testbed*.mail | extracttbmail.pl > tb.mail
#

#
# All messages must match this
# Note the ?: to avoid clustering since we use this string in a clustering
# context below.
#
my $TB_RE = '^(?:TESTBED|EMULAB\.NET): ';

#
# Things we explicitly don't care about.
# All will start with TB_RE prefix which has been stripped.
#
my @ignore_pat = (
  '^New Project',
  '^New Group',
  '^New Node',
  '^Delete User',
  '^Project',
  '^Group',
  '^User',
  '^Account',
  '^Node',
  '^Virtual Node',
  '^Password',
  '^Genlastlog',
  '^Exports Setup',
  '^Frisbee',
  '^Image Creation',
  '^Membership',
  '^Mysqld Watchdog',
  '^Named Setup',
  '^Reload Daemon',
  '^Repositioning Daemon',
  '^Stated ',
  '^SFS ',
  '^SSH ',
  '^W[Ee][Bb] ',
  '^Widearea',
  '^Your CD ',
  '^changeuid ',
  '^genelists',
  '^jabbersetup',
  '^mkproj ',
  '^mkgroup ',
  '^modgroups',
  '^mkacct ',
  '^mkusercert ',
  '^os_select ',
  '^tbacct ',
  '^rmproj ',
  '^rmgroup ',
  '^rmuser ',
  '^snmpit',
  '^setgroups ',
  '^addpubkey ',
  '^DB',
  '^TBExpt',
  '^Batch Mode Experiment Removed',
  '^Swap or Terminate Request',
  '^Survey ',
  '^Failed',
  '^plab',
  '^Lease',
  '^Sliver',
  '^Testsuite',
  '^Linktest',
  '^Testbed Audit',
  '^WARNING: Experiment Configuration',
  '^Experiment Configure Failure',
  '^Experiment Swapin Failure',
  '^Files accessed by',
  '^Failure importing',
  '^Image Creation Failure',
  '^Event System Failure',
  '^Termination Failure',
  '^Idle-Swap Warning',
  '^Auto-Swap Warning',
  '^Max Duration Warning',
  '^Parser Exceeded CPU Limit',
  '^Please tell us about your experiment',
  '^Login ID requested by ',
  '^No power outlet ',
  '^Power (?:cycle|on) nodes:',
  '^Robot Lab',
  '^Grab WebCam',
  '^OPS has rebooted',
  '^DHCPD died',
  '^USRP ',
  '^ElabInElab',
  '^Panic Button',
  '^Mailman list',
  '^WARNING: power controller',
  '^Duplicates in plab',
  '^Plab',
  '^WARNING: PLAB',
  'plab nodes revived\.',
  '[Nn]ew plab node',
  'Swap or Terminate Experiment',
  '^Firewalled experiment Swapout failed',
  '^Batch Mode.* Failure',
  '^Swap ?(?:in|out|restart|modify) Failure',
  '.* swap settings changed',
  '.* User Key',
  '.* Project Join Request',
  '.* Email Lists',
  '.* down\?',
  '.*acct-ctrl Failed',
  '.* WA node ',
  '^\d+ (?:virtual )?nodes? (?:is|are) down',
  '^\d+ PCs? idle \d+ hour',
  '\(fwd\)$',
);

my $msgs = 0;
my $goodmsgs = 0;
my $dupmsgs = 0;
my @curmsg = ();
my %seen;
my $whine = 0;

sub processmsg(@);

# compile REs for speed
my @ignore_re = map { qr/$_/ } @ignore_pat;

while ($line = <STDIN>) {
    #
    # Found the start of a message.
    # Process the message that just ended before starting afresh
    #
    if ($line =~ /^From /) {
	processmsg(@curmsg)
	    if (@curmsg);
	@curmsg = ();
    }
    push(@curmsg, $line);
}
processmsg(@curmsg)
    if (@curmsg);

print STDERR "Saved $goodmsgs of $msgs messages";
print STDERR ", ($dupmsgs duplicates)"
    if ($dupmsgs);
print STDERR "\n";
exit(0);

sub processmsg(@) {
    my (@lines) = @_;

    my $msgid = "";
    my $wantthis = 0;
    my $seensubject = 0;
    for my $line (@lines) {
	if ($line =~ /^Message-[Ii][Dd]: (<.*>)$/) {
	    $msgid = $1;
	    last if ($seensubject);
	    next;
	}
	if ($line =~ /^Subject: (.*)/) {
	    if (isgoodsubject($1)) {
		#
		# To filter out old messages from MINI, Leigh's home test
		# and other forwarded messages, we restrict the message
		# ID to cs.utah.edu or emulab.net
		#
		if ($msgid) {
		    if ($msgid !~ /\.emulab\.net/ &&
			$msgid !~ /\.cs\.utah\.edu/) {
			last;
		    }
		    if ($msgid =~ /mini\.emulab\.net/) {
			last;
		    }
		}
		$wantthis = 1;
		if ($seensubject) {
		    print STDERR "*** $msgid: multiple subject lines\n".
			"    Original subject: $seensubject".
			"    Check forwarded message filter.\n";
		}
	    }
	    $seensubject = $line;
	    last if ($msgid ne "");
	    next;
	}
    }

    if ($msgid ne "") {
	# filter out duplicates
	if ($seen{$msgid}) {
	    print STDERR "Duplicate message $msgid ignored\n"
		if ($whine);
	    $dupmsgs++;
	    $msgs++;
	    return;
	}
	$seen{$msgid} = 1;
    }

    if ($wantthis) {
	# Output lines
	for my $line (@lines) {
	    print $line;
	}
	$goodmsgs++;
    }
    $msgs++;
}

sub isgoodsubject($)
{
    my ($str) = @_;
    my $pat;

    # must start with the magic string
    if ($str !~ /$TB_RE(.*)/o) {
	return 0;
    }
    $str = $1;

    for $pat (@ignore_re) {
	if ($str =~ /$pat/) {
	    return 0;
	}
    }

    return 1;
}
