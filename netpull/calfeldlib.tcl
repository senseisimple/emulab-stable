######################################################################
# calfeldlib.tcl
#
# Provides some basic routines that show up all over the place.
#
# perror <msg> - Displays a message to stderr if errors are turned on.
# warn <msg> - Displays a message to stderr if warnings are turned on.
# debug <msg>
# verbose <msg> <type> - Displays a message to stdout if the verbose
#   type is in the outputable types.
# setoutput <type> <state> - Sets the displaying of errors, warnings,
#   and the verbose level.
#
# checkname <name> - Returns 1 if name ok, 0 otherwise
# listinsert <list> <index> <element> - Does a list insertion with replacement.
# delarray <arrayname> - Deletes the array named
# lremove <list> <element> - Removes an element from <list>
# lpop <list> - Pops from a list
# ldiff <a> <b> - returns <a not b> <a and b> <b not a>
######################################################################

namespace eval CalfeldLib {
# This is the date of last release (epoch+VERSION)
# 9/9/99
set VERSION 936900465

namespace export perror
namespace export warn
namespace export verbose
namespace export setoutput
namespace export checkname
namespace export debug
namespace export listinsert
namespace export delarray
namespace export lremove
namespace export lpop
namespace export ldiff

######################################################################
# Variables
######################################################################

##################################################
# Output
#  Array indexed by error|warn|verbose that holds the levels of each.
##################################################
variable Output
set Output(error) 1
set Output(warn) 1
set Output(verbose) {ALL}
set Output(debug) {}

######################################################################
# Code
######################################################################

##################################################
# perror <msg>
#  Displays <msg> to stderr if error level is > 0
##################################################
proc perror {msg} {
    variable Output

    if {$Output(error) > 0} {
	puts stderr "ERROR: $msg"
    }
}

##################################################
# warn <msg>
#  Displays <msg> to stderr if warn level is > 0
##################################################
proc warn {msg} {
    variable Output
    
    if {$Output(warn) > 0} {
	puts stderr "WARNING: $msg"
    }
}

##################################################
# verbose <msg> <type>
#  Displays <msg> to stdout if verbose level is in Output(verbose)
# XXX - use array to make this more efficient.
##################################################
proc verbose {msg type} {
    variable Output
    
    if {$Output(verbose) == "ALL" || 
	[lsearch $Output(verbose) $type] != -1} {
	puts "V: $msg"
    }
}

##################################################
# debug <msg> <type>
#  Displays <msg> to stdout if debug level is in Output(debug)
# XXX - use array to make this more efficient
##################################################
proc debug {msg type} {
    variable Output
    
    if {$Output(debug) == "ALL" || 
	[lsearch $Output(debug) $type] != -1} {
	puts "D: $msg"
    }
}


##################################################
# setoutput <type> <value>
#  Just a simple access routine to the Output varibale.
##################################################
proc setoutput {type value} {
    variable Output

    set Output($type) $value
}

##################################################
# checkname <name>
#  Checks for a valid name, currently just avoids spaces.
##################################################
proc checkname {name} {
    # XXX: add checking for [],{},",\t,\n, etc.
    if {[regexp { } $name] == 1} {return 0} else {return 1}
}

##################################################
# listinsert <list> <index> <element>
#   This inserts <element> into <list> at position <index>, filling
# with {}'s as necessary and returns the new list.  Different from
# linsert in that it replaces elements rather than expanding the list.
##################################################
proc listinsert {list index element} {
    set length1 [llength $list]
    set length2 [expr $index + 1]
    if {$length1 > $length2} {
	set newlength $length1
    } else {
	set newlength $length2
    }
    
    set newlist {}
    for {set i 0} {$i < $newlength} {incr i} {
	if {$i == $index} {
	    lappend newlist $element
	} else {
	    lappend newlist [lindex $list $i]
	}
    }

    return $newlist
}

##################################################
# delarray <arrayname>
#  Complete deletes the array named.
##################################################
proc delarray {arrayname} {
    upvar $arrayname A

    foreach ind [array names A] {
	unset A($ind)
    }
}

##################################################
# lremove <list> <element>
#  Removes <element> from <list> (name of list)
##################################################
proc lremove {list element} {
    upvar $list L

    set i [lsearch $L $element]
    if {$i == -1} {return}
    set L [lreplace $L $i $i]
}

##################################################
# lpop <list>
##################################################
proc lpop {list} {
    upvar $list L
    set ret [lindex $L 0]
    set L [lrange $L 1 end]
    return $ret
}

##################################################
# ldiff <a> <b>
#  Returns list of three lists: a-b, intersection, b-a
##################################################
proc ldiff {a b} {
    set a [lsort $a]
    set b [lsort $b]
    set ab {}
    set ba {}
    set int {}
    while {$a != {} && $b != {}} {
	set ae [lindex $a 0]
	set be [lindex $b 0]
	if {$ae == $be} {
	    # in both lists
	    lappend int $ae
	    lpop a
	    lpop b
	} else {
	    if {$ae < $be} {
		# read from ae until >=
		while {$a != {} && $ae < $be} {
		    lappend ab $ae
		    lpop a
		    set ae [lindex $a 0]
		}
	    } else {
		# read from be until >=
		while {$b != {} && $be < $ae} {
		    lappend ba $be
		    lpop b
		    set be [lindex $b 0]
		}
	    }
	}
    }
    if {$a != {}} {
	set ab [concat $ab $a]
    } elseif {$b != {}} {
	set ba [concat $ba $b]
    }
    return [list $ab $int $ba]
}

}