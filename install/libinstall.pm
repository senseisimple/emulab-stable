#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2003-2007 University of Utah and the Flux Group.
# All rights reserved.
#

#
# A simple library for use in the installation scripts, to make them seem a
# little more legitimate, instead of the quick hacks they are.
#
use POSIX qw(strftime);

#
# Make sure that output gets printed right away
#
$| = 1;

#
# Magic string that shows up in files we've already edited 
#
my $MAGIC_STRING = "testbed installation process";

my $MAGIC_TESTBED_START = "The follwing lines were added by the $MAGIC_STRING";
my $MAGIC_TESTBED_END = "End of testbed-added configuration";

sub MAGIC_TESTBED_START { $MAGIC_TESTBED_START; }
sub MAGIC_TESTBED_END { $MAGIC_TESTBED_END; }

#
# Some programs we may call
#
my $FETCH = "/usr/bin/fetch";

#
# Let's pretend perl's exception mechanism has a sane name for the function
# that raises an exception
#
sub throw(@) {
    die @_,"\n";
}
 
#
# Start a new installation phase
#
sub Phase($$$) {
    my ($name, $descr, $coderef) = @_;

    push @libinstall::phasestack, $name;
    push @libinstall::descrstack, $descr;
    
    #
    # Okay, this will probably earn me my own special circle in Perl Hell. Use
    # dynamic scoping to allow data to be passed inbetween indirectly recursive
    # calls of Phase(). hasSubPhase is used to determine if we were called
    # recursively in the eval(). skipped and nonSkipped let us figure out if
    # all child calls were skipped. But, we have to give this information to
    # our parent _after_ we declare our own. So, we use references. Damn!
    #
 
    $hasSubPhase++;
    my $firstSubPhase = ($hasSubPhase == 1);
    local $hasSubPhase = 0;
    
    my $parentNonSkipped = \$nonSkipped;
    my $parentSkipped = \$skipped;
    local $nonSkipped = 0;
    local $skipped = 0;
    
    local $depth = ($depth? $depth +1 : 1);
    my $isSubPhase = ($depth > 1);

    my $descrstring = "| " x ($depth -1) . $descr;
    if ($firstSubPhase) {
	print "\n";
    }
    printf "%-50s", $descrstring;

    #
    # Clear these, as we don't want to see the outputs of previous phases
    #
    @libinstall::lastExecOutput = ();
    $libinstall::lastCommand = undef;

    #
    # Cool! TWO levels of Perl Hell just for me!
    # We pretend we have real exceptions, here. The way this works, we _expect_
    # something to call die(), which is how you throw exceptions in perl. Neat,
    # huh? The three calls below (Phase?*()) are the ones that do this.
    #
    eval { &$coderef(); };

    #
    # Prepare for printing!
    #
    my $message = "";
    my $die = 0;
    if ($hasSubPhase) {
	print  "| " x ($depth -1) . "+-" . "--" x (24 - $depth) . "> ";
    }

    #
    # Check the exception thrown by the eval()
    #
    SWITCH: for ($@) {
	(/^skip$/) && do {
	    print "[ Skipped ($libinstall::reason) ]\n";
	    $$parentSkipped++;
	    $libinstall::phaseResults{$name} = $_;
	    last SWITCH;
	};

	(/^fail$/) && do {
	    print "[ Failed    ]\n";
	    $$parentNonSkipped++;
	    $libinstall::phaseResults{$name} = $_;
	    $die = 1;
	    $message = "Installation failed: $libinstall::reason";
	    if ($isSubPhase) {
		#
		# Propagate failure up the tree
		#
		PhaseFail($libinstall::reason);
	    }
	    last SWITCH;
	};

	(undef || /^$/ || /^succeed$/) && do {
	    #
	    # If we're a parent, and all sub-phases got skipped, we did too
	    #
	    my $stamp = POSIX::strftime("%H:%M:%S", localtime());
	    
	    if ($hasSubPhase && $skipped && ($nonSkipped == 0)) {
		print "[ Skipped   ] ($stamp)\n";
		$libinstall::phaseResults{$name} = "skip";
		$$parentSkipped++;
	    } else {
		print "[ Succeeded ] ($stamp)\n";
		$$parentNonSkipped++;
		$libinstall::phaseResults{$name} = "succeed";
	    }
	    last SWITCH;
	};

	#
	# Default case - shouldn't get here, unless something called die with
	# the wrong value
	#
	print "[ ERROR     ]\n";
	$$parentNonSkipped++;
	$message = "Internal error - Bad exception:\n$_";
	$libinstall::phaseResults{$name} = "fail";
        $die = 1;
	if ($isSubPhase) {
	    #
	    # Propagate failure up the tree
	    #
	    PhaseFail($message);
	}
    }

    #
    # If we decided that we need to die, do that now
    #
    if ($die) {
	print "\n\n##### Installation failed in phase $name. The error was:\n";
	print "$message\n";
	PrintPhaseTrace();
	PrintLastOutput();
	print "Please send the above output to testbed-ops\@emulab.net\n";
	exit -1;
    }

    #
    # Pop ourselves off the phase stack!
    #
    pop @libinstall::phasestack;
    pop @libinstall::descrstack;

}

#
# Signal that the current phase has suceeded
#
sub PhaseSucceed(;$) {
    ($libinstall::reason) = (@_);
    if (!$libinstall::reason) {
	$libinstall::reason = "succeeded";
    }
    throw "succeed";
}

#
# Signal that the current phase has failed
#
sub PhaseFail($) {
    ($libinstall::reason) = (@_);
    throw "fail";
}

#
# Signal that the current phase is being skipped
#
sub PhaseSkip($) {
    ($libinstall::reason) = (@_);
    throw "skip";
}

#
# Check to see if a previous phase was skipped. Returns 1 if it was, 0 if not
#
sub PhaseWasSkipped($) {
    my ($phase) = (@_);
    return ($libinstall::phaseResults{$phase} &&
    	($libinstall::phaseResults{$phase} =~ /^skip$/));
}

#
# Check to see if the phase is already done, as evidenced by the existance of
# a file
#
sub DoneIfExists($) {
    my ($filename) = @_;
    if (!$filename) { PhaseFail("Bad filename passed to DoneIfExists"); }
    if (-e $filename) {
	PhaseSkip("File already exists");
    }
}

#
# Same as above, but done if it doesn't exist
#
sub DoneIfDoesntExist($) {
    my ($filename) = @_;
    if (!$filename) { PhaseFail("Bad filename passed to DoneIfExists"); }
    if (!-e $filename) {
	PhaseSkip("File does not exist");
    }
}

#
# Check to see if the phase is already done, as evidenced by the existance of
# comments within a file
#
sub DoneIfEdited($) {
    my ($filename) = @_;
    if (!$filename) { PhaseFail("Bad filename passed to DoneIfEdited"); }
    open(FH,$filename) or return;
    if (grep /$MAGIC_STRING/, <FH>) {
        close(FH);
	PhaseSkip("File has already been edited");
    }
    close(FH);
}

#
# Check to see if the phase is already done, as evidenced by the fact that two
# files are identical
#
sub DoneIfIdentical($$) {
    my ($filename1,$filename2) = @_;
    if (!$filename1 || !$filename2) {
	PhaseFail("Bad filename passed to DoneIfIdentical");
    }
    if (!-e $filename1 || !-e $filename2) {
	return;
    }
    if (!ExecQuiet("cmp -s $filename1 $filename2")) {
	PhaseSkip("Files $filename1 and $filename2 are identical");
    }
}

#
# Check to see if filesystem already mounted
#
sub DoneIfMounted($)
{
    my ($dir) = @_;

    #
    # Grab the output of the mount command and parse. 
    #
    if (! open(MOUNT, "/sbin/mount|")) {
	PhaseFail("Cannot run mount command");
    }
    while (<MOUNT>) {
	if ($_ =~ /^([-\w\.\/:\(\)]+) on ([-\w\.\/]+) \((.*)\)$/) {
	    # Search for nfs string in the option list.
	    foreach my $opt (split(',', $3)) {
		if ($opt eq "nfs") {
		    if ($dir eq $2) {
			close(MOUNT);
			PhaseSkip("NFS dir already mounted");
		    }
		}
	    }
	}
    }
    close(MOUNT);
    return;
}

#
# Append some text to a configuration file, with a special testbed tag that 
# will help inform the user of what's been done, and help future invocations
# tell that this has already been done. Returns undef if it succeeds, or
# an error string if it fails
# TODO - handle alternate comment characters
# TODO - handle files that it's OK to create
#
sub AppendToFile($@) {
    my ($filename, @lines) = @_;
    if (!-e $filename) {
	return "File $filename does not exist";
    }
    open(FH,">>$filename") or return "Unable to open $filename for ".
	"writing: $!";

    print FH "# $MAGIC_TESTBED_START\n";
    print FH map "$_\n", @lines;
    print FH "# $MAGIC_TESTBED_END\n";

    close(FH);

    return undef;
}

#
# Same as above, but call PhaseFail on failure
#
sub AppendToFileFatal($@) {
    my ($filename,@lines) = @_;
    my $error = AppendToFile($filename,@lines);
    if ($error) {
	PhaseFail($error);
    }
}

#
# Create a new file (must not already exist), and fill it with the given
# contents. Returns undef if successful, or an error message otherwise.
#
sub CreateFile($;@) {
    my ($filename,@lines) = @_;
    if (-e $filename) {
	return "File $filename already exists";
    }
    open(FH,">$filename") or return "Unable to open $filename for ".
	"writing: $!";

    if (@lines) {
	print FH map "$_\n", @lines;
    }

    close FH;
    return undef;    
}

#
# Same as above, but call PhaseFail on failure
#
sub CreateFileFatal($@) {
    my ($filename,@lines) = @_;
    my $error = CreateFile($filename,@lines);
    if ($error) {
	PhaseFail($error);
    }
}

#
# Execute a program, hiding its stdout and stderr from the user, but saving
# them for later use. Returns the exit value of the program if used in scalar
# context, or an array composed of the exit status and output if used in array
# context.
#
sub ExecQuiet(@) {
    #
    # Use a pipe read, so that we save away the output
    #
    my $commandstr = join(" ",@_);
    my @output = ();
    open(PIPE,"$commandstr 2>&1 |") or return -1;
    while (<PIPE>) {
	push @output, $_;
    }
    close(PIPE);

    my $exit_value  = $? >> 8;

    @libinstall::lastExecOutput = @output;
    $libinstall::lastCommand = $commandstr;

    if (wantarray) {
	return ($exit_value, @output);
    } else {
	return $exit_value;
    }
}

#
# Same as the above, but the current phase fails if the program returns a non-0
# exit status.
#
sub ExecQuietFatal(@) {
    if (ExecQuiet(@_)) {
	PhaseFail("Unable to execute: ");
    }
}

#
# HUP a daemon, if it's PID file exists. If we can't kill it, we assume that
# it's because it wasn't running, and skip the phase. Fails if it has trouble
# reading the pid file.
# Takes the name of the daemon as an argument, and assumes
# that the pid file is /var/run/$name.pid
#
sub HUPDaemon($) {
    my ($name) = @_;
    my $pidfile = "/var/run/$name.pid";
    PhaseSkip("$name is not running") unless (-e $pidfile);
    open(PID,$pidfile) or PhaseFail("Unable to open pidfile $pidfile");
    my $pid = <PID>;
    chomp $pid;
    close PID;

    PhaseFail("Bad pid ($pid) in $pidfile\n") unless ($pid =~ /^\d+$/);
    if (!kill 1, $pid) {
	PhaseSkip("$name does not seem to be running");
    }
}

#
# Fetch a file from the network, using any protocol supported by fetch(1).
# Arguments are URL and a local filename. Retunrns 1 if succesful, 0 if not.
#
sub FetchFile($$) {
    my ($URL, $localname) = @_;
    if (ExecQuiet("$FETCH -o $localname $URL")) {
	return 0;
    } else {
	return 1;
    }
}

#
# Same as above, but failure is fatal
#
sub FetchFileFatal($$) {
    my ($URL, $localname) = @_;
    if (!FetchFile($URL,$localname)) {
	PhaseFail("Unable to fetch $URL");
    }
}

#
# Locate the proper version of a package to install by looking
# at the available package tarballs.
#
# Must be called "in phase" since we will PhashFail on errors.
#
sub GetPackage($$) {
    my ($prefix, $packagedir) = @_;

    PhaseFail("Must provide -p (packagedir) argument!")
	if (!$packagedir);

    my $pname = `ls $packagedir/$prefix-*.tbz 2>/dev/null`;
    if ($?) {
	$pname = `ls $packagedir/$prefix-*.tgz 2>/dev/null`;
	PhaseFail("Cannot find $prefix package in $packagedir!")
	    if ($?);
    }
    chomp($pname);

    return $pname;
}

#
# Print out the phase stack that got us here
#
sub PrintPhaseTrace() {
    print "-------------------------------------------------- Phase Stack\n";
    my @tmpphase = @libinstall::phasestack;
    my @tmpdescr = @libinstall::descrstack;
    my ($phase, $descr);
    while (@tmpdescr) {
	($phase, $descr) = (pop @tmpphase, pop @tmpdescr);
	printf "%-10s %-50s\n", $phase, $descr;
    }
    print "--------------------------------------------------------------\n";
}

#
# Print out the ouput of the last command that ran
#
sub PrintLastOutput() {
    if (!$libinstall::lastCommand) {
	return;
    }
    print "------------------------------------------ Last Command Output\n";
    print "Command: $libinstall::lastCommand\n";
    print @libinstall::lastExecOutput;
    print "--------------------------------------------------------------\n";
}

#
# Get me a secret
#
sub GenSecretKey()
{
    my $key=`/bin/dd if=/dev/urandom count=128 bs=1 2> /dev/null | /sbin/md5`;
    chomp($key);
    return $key;
}

1;
