########################################################################
#
# set.pl
#
# A collection of set-manipulation routines for use with perl.
#
########################################################################

# Input: length(A), A, B
# Output: everything that is in only one of A or B

sub set_xor {
    local($len1) = @_;
    local(@set1, @set2, $val1, $val2);
    local(@diff, $found);

    @set1 = @_[1 .. $len1];
    @set2 = @_[$len1+1 .. $#_];

#    print "Set 1 : ", join(" ", @set1), "\n";
#    print "Set 2 : ", join(" ", @set2), "\n";

    foreach $val1 (@set1) {
	$found = 0;
	inner: foreach $val2 (@set2) {
	    if ($val1 eq $val2) {
		$found = 1;
		last inner;
	    }
	}
	if ($found == 0) {
	    push(@diff, $val1);
	}
    }

    foreach $val1 (@set2) {
	$found = 0;
	inner: foreach $val2 (@set1) {
	    if ($val1 eq $val2) {
		$found = 1;
		last inner;
	    }
	}
	if ($found == 0) {
	    push(@diff, $val1);
	}
    }

#    print "Xor   : ", join(" ", @diff), "\n";
    return(@diff);
}

########################################################################

# Input: length(A), A, B
# output: A - B

sub set_diff {
    local($len1) = @_;
    local(@set1, @set2, $val1, $val2);
    local(@diff, $found);

    @set1 = @_[1 .. $len1];
    @set2 = @_[$len1+1 .. $#_];

#    print "Set 1 : ", join(" ", @set1), "\n";
#    print "Set 2 : ", join(" ", @set2), "\n";

    foreach $val1 (@set1) {
	$found = 0;
	inner: foreach $val2 (@set2) {
	    if ($val1 eq $val2) {
		$found = 1;
		last inner;
	    }
	}
	if ($found == 0) {
	    push(@diff, $val1);
	}
    }

#    print "Diff  : ", join(" ", @diff), "\n";
    return(@diff);
}

########################################################################
#
# Input: A
# Returns set of unique elements in A.

sub set_unique {
    local(@set) = @_;
    local($val1, $val2, $found, @unique);

    foreach $val1 (@set) {
	if (!defined $val1) {
	    next;
	}
	$found = 0;
	inner: foreach $val2 (@unique) {
	    if (defined $val2 && $val1 eq $val2) {
		$found = 1;
		last inner;
	    }
	}
	if ($found == 0) {
	    push(@unique, $val1);
	}
    }
    return(@unique);
}

sub set_union {
    local(@A, @B) = @_;
    return(&set_unique(@A, @B));
}

########################################################################
#
# Input: length(A), A, B
# Returns: intersecion of A and B

sub set_intersect {
    local($len1) = @_;
    local(@set1, @set2);
    local($val1, $val2, @result);

    @set1 = &set_unique(@_[1 .. $len1]);
    @set2 = &set_unique(@_[$len1+1 .. $#_]);

    foreach $val1 (@set1) {
	foreach $val2 (@set2) {
	    if ($val1 eq $val2) {
		push(@result, $val1);
	    }
	}
    }
    return(@result);
}

########################################################################

sub set_ismember {
    local($item, @list) = @_;
    local($val);

    foreach $val (@list) {
	if ($val eq $item) {
	    return 1;
	}
    }
    return 0;
}

########################################################################

1;
