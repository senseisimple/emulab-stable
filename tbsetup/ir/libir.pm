######################################################################
# libir.pl
#
# API:
#  ir_read <file> 
#  ir_write <file> 
#  ir_get <path> => <contents>
#  ir_set <path> <contents>
#  ir_list <path> => <list of subsections>
#  ir_exists <path> => 1 | ''
#  ir_type <path> => 'file' | 'dir'
#
# Contents are alays newline seperated strings of lines.  
# CAVEAT: Be sure to have all contents end with a newline.
#
# All routines throw die's on errors.
#
# Until ir_read succesfully completes all other routines have undefined
# behavior.
#
# <path> is a / separated path (ex. /topology/nodes).  / will get you
# the toplevel.
#
# LIMITATIONS/TODO:
#  ir_read/write does not preserve any comments.
#  There is no mechanism for creating new sections.  
#  Should check and add trailing newline for ir_set
#
# INTERNALS:
#  The IR file is stored in the associative array contents.  The key is
# the pathname and the value is the text of the section.  The path followed
# by an underscore contains the type of the section.
#  For simplicity directory structure is also a newline seperated list.
######################################################################

# tail and dirname are equivalents of the tcl built in functions
# file tail and file dirname.  tail returns the last component of a
# / seperated list and dirname returns the first n-1 components of
# a /seperated list (length n)
sub libir'tail {
    @t = split('\/',$_[0]);
    return $t[$#t];
}

sub libir'dirname {
    local($ret);
    @t = split('\/',$_[0]);
    $ret = join('/',@t[0..$#t - 1]);
    if ($ret eq "") {
	$ret = "/";
    }
    return $ret;
}

# This functions take a filename is an argument and reads the contents
# into the libir'contents variable for later use by the other routines.
# ir_read returns '' on success and an error message on any error.  Until
# this routine succesfully completes the behavior of all other routines
# is undefined.
sub ir_read {
    package libir;

    local($cursec) = '/';
    local($curcontents) = '';
    local($found) = 0;
    local(@line) = {};
    local($curtail) = '';

    open(FILE, "$_[0]") || die "ERROR: (ir_read) Could not open IR file!";

    %contents = ();

    while (<FILE>) {
	# skip comments
	chop;
	/^\#/ && next;
	if ($_ eq "") {
	    next;
	}
	@line = split;
	
	# There are three cases we need to deal with.  The start of a section,
	# the end of a section, and the interior of a section.
	if ($line[0] eq "START") {
	    $contents{$cursec . '_'} = 'dir';
	    $contents{$cursec} .= $line[1] . "\n";

	    if ($cursec eq '/') {
		$cursec = '/' . $line[1];
	    } else {
		$cursec = $cursec . '/' . $line[1];
	    }
	    $contents{$cursec . '_'} = 'file';
	} elsif ($line[0] eq "END") {
	    $curtail = &tail($cursec);
	    if ($curtail ne $line[1]) {
		close FILE;
		die "ERROR: (ir_read) END did not match START ($curtail != $line[1])";
	    }
	    if ($contents{$cursec . '_'} eq 'file') {
		$contents{$cursec} = $curcontents;
	    }
	    $cursec = &dirname($cursec);
	    $curcontents = '';
	} else {
	    if ($contents{$cursec . '_'} eq 'dir') {
		close FILE;
		die "ERROR: (ir_read) Found data outside of section ($line)";
	    }
	    $curcontents .= join(' ',@line) . "\n";
	}
    }
    if ($cursec ne '/') {
	close FILE;
	die "ERROR: (ir_read) Missing ENDs ($cursec)";
    }

    close FILE;

    return 1;
};

# This functions transfer the contents array to a file in IR format.  It
# does this via the recursive procedure _ir_write.
sub ir_write {
    package libir;

    open(FILE,">$_[0]");
    
    &_ir_write("/");

    close FILE;
}
sub libir'_ir_write {
    package libir;

    local($rpath,$child);
    if ($_[0] ne "/") {
	print FILE "START " . &tail($_[0]) . "\n";
    }
    if (&main'ir_type($_[0]) eq "dir") {
	if ($_[0] eq '/') {
	    $rpath = "";
	} else {
	    $rpath = $_[0];
	}
	foreach $child (&main'ir_list($_[0])) {
	    &_ir_write($rpath . "/" . $child);
	}
    } else {
	print FILE &main'ir_get($_[0]);
    }
    if ($_[0] ne "/") {
	print FILE "END " . &tail($_[0]) . "\n";
    }

    return 1;
};


# Just an accessor routine into the contents array.
sub ir_get {
    package libir;
    if (! defined($contents{$_[0]})) {
	die "ERROR: (ir_get) No such path $_[0]\n";
    }
    if ($contents{$_[0] . '_'} ne "file") {
	die "ERROR: (ir_get) $_[0] is a directory, not a file\n";
    }

    return $contents{$_[0]};
};

# Another accessor routine.
sub ir_set {
    package libir;

    if (! defined($contents{$_[0]})) {
	die "ERROR: (ir_set) No such path $_[0]\n";
    }
    if ($contents{$_[0] . '_'} ne "file") {
	die "ERROR: (ir_set) $_[0] is a directory, not a file\n";
    }

    $contents{$_[0]} = $_[1];
};

# This returns an array of the contents of a directory in the IR.
sub ir_list {
    package libir;
    if (! defined($contents{$_[0]})) {
	die "ERROR: (ir_list) No such path $_[0]\n";
    }
    if ($contents{$_[0] . '_'} ne "dir") {
	die "ERROR: (ir_list) $_[0] is a file, not a directory\n";
    }

    return split("\n",$contents{$_[0]});
};

# This just checks to see if a path exists.  Basically a wrapper for 
# defined()
sub ir_exists {
    package libir;

    return defined($contents{$_[0]});
};

# This returns the type (file | dir) of a path.
sub ir_type {
    package libir;

    if (! defined($contents{$_[0] . '_'})) {
	die "ERROR: (ir_type) No such path $_[0]\n";
    }

    return $contents{$_[0] . '_'};
}

1;
