#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#

#
# A simple library for use in the installation scripts, to make them seem a
# little more legitimate, instead of the quick hacks they are.
#

#
# Magic string that shows up in files we've already edited 
#
my $MAGIC_STRING = "testbed installation process";

my $MAGIC_TESTBED_START = "The follwing lines were added by the $MAGIC_STRING";
my $MAGIC_TESTBED_END = "End of testbed-added configuration";

sub MAGIC_TESTBED_START { $MAGIC_TESTBED_START; }
sub MAGIC_TESTBED_END { $MAGIC_TESTBED_END; }

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
	print  "| " x ($depth -1) . "+-" . "--" x (25 - $depth);
    }

    #
    # Check the exception thrown by the eval()
    #
    SWITCH: for ($@) {
	(/^skip$/) && do {
	    print "[ Skipped   ]\n";
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
	    if ($hasSubPhase && $skipped && ($nonSkipped == 0)) {
		print "[ Skipped   ]\n";
		$libinstall::phaseResults{$name} = "skip";
		$$parentSkipped++;
	    } else {
		print "[ Succeeded ]\n";
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
    if (-e $filename) {
	PhaseSkip("File $filename already exists");
    }
}

#
# Check to see if the phase is already done, as evidenced by the existance of
# comments within a file
#
sub DoneIfEdited($) {
    my ($filename) = @_;
    open(FH,$filename) or return;
    if (grep /$MAGIC_STRING/, <FH>) {
	PhaseSkip("File $filename has already been edited\n");
    }
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
# them for later use. Returns the exit value of the program.
#
sub ExecQuiet(@) {
    my @commnads = @_;

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

    return $exit_value;
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

1;
