######################################################################
# program.tcl
#
# This defines the local program agent.
#
######################################################################

Class Program -superclass NSObject

namespace eval GLOBALS {
    set new_classes(Program) {}
}

Program instproc init {s} {
    global ::GLOBALS::last_class

    $self set sim $s
    $self set node {}
    $self set command {}

    # Link simulator to this new object.
    $s add_program $self

    set ::GLOBALS::last_class $self
}

Program instproc rename {old new} {
    $self instvar sim

    $sim rename_program $old $new
}

# updatedb DB
# This adds rows to the virt_trafgens table corresponding to this agent.
Program instproc updatedb {DB} {
    var_import ::GLOBALS::pid
    var_import ::GLOBALS::eid
    var_import ::GLOBALS::objtypes
    $self instvar node
    $self instvar command

    if {$node == {}} {
	perror "\[updatedb] $self has no node."
	return
    }
    if {$command == {}} {
	perror "\[updatedb] $self has no command."
	return
    }

    sql exec $DB "insert into virt_agents (pid,eid,vnode,vname,objecttype) values ('$pid','$eid','$node','$self','$objtypes(PROGRAM)')";
}

