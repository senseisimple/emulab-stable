######################################################################
# oslib.tcl
#
# os <command> <args>
# 
# init
# end
# querydb <node> - Returns a node_state (see below)
# querynode <node> - Returns a node_state (see below)
# listbases
# listdeltas <base> - Returns list of deltas
# querybase <base> - Returns {os, ver, extra, desc, path}
# querydelta <delta> - Returns {name, desc, path}
# db - Returns sql connection.
#
# node_state = {base {deltas}}
######################################################################

if {[file dirname [info script]] == "."} {
    load ../lib/sql.so
} else {
    load [file dirname [file dirname [info script]]]/lib/sql.so
}

namespace eval TB_OS {

namespace export os;

variable commands {
    init end 
    querydb querynode 
    listbases listdeltas
    querybase querydelta
    db
}
variable DB {}

##################################################
# os <command> <args>
# Main interface into this library.  See header for list of
# commands and args.
##################################################
proc os {command args} {
    variable commands
    variable DB

    if {[lsearch $commands $command] == -1} {
	error "$command not a valid os command."
    }
    if {$DB == {} && $command != "init"} {
	error "Need to run 'os init' first."
    }
    
    return [eval "[namespace current]::$command $args"]
}

##################################################
# init
# This initalizes a connection to the database.
##################################################
proc init {} {
    variable DB

    set DB [sql connect]
    sql selectdb $DB tbdb
}

##################################################
# end
# This disconnects from the database.
##################################################
proc end {} {
    variable DB
    sql disconnect $DB
    set DB {}
}

##################################################
# querydb <node>
# Returns the base and deltas for <node>
##################################################
proc querydb {node} {
    variable DB

    sql query $DB "select image_id from SW_table where node_id=\"$node\""
    set base [sql fetchrow $DB]
    if {$base == {}} {return {}}
    sql endquery $DB

    sql query $DB "select fix_id from fixed_list where node_id=\"$node\""
    set fix_ids {}
    while {[set row [sql fetchrow $DB]] != {}} {
	lappend fix_ids $row
    }
    sql endquery $DB

    return [list $base $fix_ids]
}

##################################################
# querynode <node>
# Queries the node about the it's state and creates a node_state.
##################################################
proc querynode {node} {
    variable DB

    # calculate OS type
    if {[catch "exec ssh -n -x $node uname -a" uname]} {
	error "$uname"
    }
    sql query $DB "select image_id, magic from disk_images"
    set os {}
    while {[set row [sql fetchrow $DB]] != {}} {
	set id [lindex $row 0]
	set magic [lindex $row 1]
	if {$magic != {}} {
	    if {[string first $magic $uname] != -1} {
		set os $id
		break
	    }
	}
    }
    sql endquery $DB

    # calculate deltas
    if {[catch "exec ssh -n -x $node rpm -qa" result]} {
	error "$result"
    }
    
    foreach pkg $result {
	sql query $DB "select fix_id from fixes_table where fix_path like \"$pkg\""
	while {[set row [sql fetchrow $DB]] != {}} {
	    lappend potential($row) $pkg
	}
	sql endquery $DB
    }

    set deltas {}
    foreach delta [array names potential] {
	sql query $DB "select fix_path from fixes table where fix_id = \"$delta\""
	set pkgs [split [sql fetchrow $DB] ,]
	if {[lsort $pkgs] == [lsort $potential($delta)]} {
	    lappend deltas $delta
	}
    }
    
    return [list $os $deltas]
}

##################################################
# listbases
# Lists all the bases in the DB.
##################################################
proc listbases {} {
    variable DB

    sql query $DB "select image_id from disk_images"
    set ret {}
    while {[set row [sql fetchrow $DB]] != {}} {
	lappend ret $row
    }
    sql endquery $DB
    return $ret
}

##################################################
# listdeltas <base>
# Returns a list of all the deltas for a given base.
##################################################
proc listdeltas {base} {
    variable DB

    sql query $DB "select fix_id from fix_compat where image_id=\"$base\""
    set ret {}
    while {[set row [sql fetchrow $DB]] != {}} {
	lappend ret $row
    }
    sql endquery $DB
    return $ret
}

##################################################
# querybase <base>
# Returns {os, ver, extra, desc} for base.
##################################################
proc querybase {base} {
    variable DB

    sql query $DB "select OS, ver, extras, img_desc, img_path from disk_images where image_id=\"$base\""
    set ret [sql fetchrow $DB]
    sql endquery $DB
    return $ret
}

##################################################
# querydelta <delta>
# Returns {name, desc, path}
##################################################
proc querydelta {delta} {
    variable DB

    sql query $DB "select fix_name, fix_desc, fix_path from fixes_table where fix_id=\"$delta\""
    set ret [sql fetchrow $DB]
    sql endquery $DB
    return $ret
}

##################################################
# db
# Plain accesor function to the DB variable.
##################################################
proc db {} {
    variable DB
    return $DB
}

}
