######################################################################
# libir.tcl
#
# API:
#  ir read <file> [<section>]
#    Note: if <section> is included ir write calls will not work.
#  ir write <file>
#  ir get <path> => <contents>
#  ir set <path> <contents> 
#  ir list <path> => <list of subsections>
#  ir exists <path> => 1 | 0
#  ir type <path> => file | dir
#
# <path> is a / seperated path (ex. /topology/nodes). / will get you
# toplevel.
#
# XXX: This removes all comments.
######################################################################

namespace eval TB_LIBIR {
  
namespace export ir

# This describes the available commands.  To add a new one just
# add a line for it and write the proc.  Format is: {proc arg1 arg2 ...}
set cmds(read) {ir_read file *section*}
set cmds(write) {ir_write file}
set cmds(get) {ir_get path}
set cmds(set) {ir_set path contents}
set cmds(list) {ir_list path}
set cmds(type) {ir_type path}
set cmds(exists) {ir_exists path}

# contents - the contents of the current ir_file.  /_ is the type 
# (dir | file).
variable contents
variable writable 1

proc ir {cmd args} {
    variable cmds

    if {! [info exists cmds($cmd)]} {
	error "Bad command $cmd, should be one of: [lsort [array names cmds]]"
    }

    set cinfo $cmds($cmd)
    
    set prc [lindex $cinfo 0]
    set arginfo [lrange $cinfo 1 end]
    set maxargs 0
    set minargs 0
    foreach arg $arginfo {
	incr maxargs
	if {[string index $arg 0] != "*"} {
	    incr minargs
	} 
    }
    
    set numargs [llength $args]
    if {$numargs < $minargs || $numargs > $maxargs} {
	error "Bad args, should be: ir $cmd $arginfo"
    }

    return [eval "$prc $args"]
}

proc ir_read {file {section {}}} {
    variable contents 
    variable writable

    # erase contents
    foreach name [array names contents] {
	unset contents($name)
    }

    # load file
    set fp [open $file r]
    
    set cursec /
    set curcontents {}
    if {$section != {}} {
	set writable 0
	set found 0
	while {[gets $fp line] >= 0} {
	    if {[regexp "^\#" $line] || $line == {}} {continue}
	    set first [lindex $line 0]
	    if {$first == "START" &&
		[lindex $line 1] == $section} {
		set found 1
		break
	    }
	}
	if {$found == 0} {
	    error "Could not find section: $section"
	}
    } else {
	set writable 1
	gets $fp line
    }
    while {1 == 1} {
	if {[regexp "^\#" $line] || $line == {}} {continue}
	set first [lindex $line 0]
	if {$first == "START"} {
	    set what [lindex $line 1]
	    set contents(${cursec}_) "dir"
	    lappend contents($cursec) $what
	    if {$cursec == "/"} {set cursec ""}
	    set cursec "$cursec/$what"
	    set contents(${cursec}_) "file"
	} elseif {$first == "END"} {
	    if {[file tail $cursec] != [lindex $line 1]} {
		error "END did not match START ([file tail $cursec] != [lindex $line 1])"
	    }
	    if {$contents(${cursec}_) == "file"} {
		set contents($cursec) $curcontents
	    }
	    set cursec [file dirname $cursec]
	    set curcontents {}
	} else {
	    if {$contents(${cursec}_) == "dir"} {
		error "Found data outside of section ($line)"
	    }
	    lappend curcontents $line
	}
	if {[gets $fp line] < 0} {break}
    }
    if {$cursec != "/"} {
	error "Missing ENDs ($cursec)"
    }

    close $fp
}

proc ir_write {file} {
    variable contents
    variable writable

    if {$writable == 0} {
	error "Can not write.  ir_read used in section form."
    }

    set fp [open $file w]
    
    _ir_write $fp /

    close $fp
}

proc _ir_write {fp path} {
    variable contents

    if {$path != "/"} {
	set name [file tail $path]
	puts $fp "START $name"
    }
    if {[ir_type $path] == "dir"} {
	if {$path == "/"} {set rpath ""} else {set rpath $path}
	foreach child [ir_list $path] {
	    _ir_write $fp $rpath/$child
	}
    } else {
	foreach line [ir_get $path] {
	    puts $fp $line
	}
    }
    if {$path != "/"} {
	puts $fp "END $name"
    }
}

proc ir_get {path} {
    variable contents

    if {! [info exists contents($path)]} {
	error "$path does not exist"
    }
    if {$contents(${path}_) != "file"} {
	error "$path is not a file"
    }

    return $contents($path)
}

proc ir_set {path newcontents} {
    variable contents

    if {! [info exists contents($path)]} {
	error "$path does not exist"
    }
    if {$contents(${path}_) != "file"} {
	error "$path is not a file"
    }

    set contents($path) $newcontents
}

proc ir_list {path} {  
    variable contents

    if {! [info exists contents($path)]} {
	error "$path does not exist"
    }
    if {$contents(${path}_) != "dir"} {
	error "$path is not a dir"
    }

    return $contents($path)
}

proc ir_exists {path} {
    variable contents

    return [info exists contents($path)]
}

proc ir_type {path} {
    variable contents

    if {! [info exists contents($path)]} {
	error "$path does not exist"
    }
    return $contents(${path}_)
}

# this is for debugging and must be called with :: syntax
proc dump {} {
    variable contents

    foreach name [array names contents] {
	puts "$name ="
	puts "$contents($name)"
    }
}
}

