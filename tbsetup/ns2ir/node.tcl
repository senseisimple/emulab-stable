######################################################################
# node.tcl
#
# This defines the Node class.  Instances of this class are created by
# the 'node' method of Simulator.  A Node is connected to a number of
# LanLinks.  Each such connection is associated with a virtual port,
# an integer.  Each virtual port also has an IP address.  Virtual
# ports start at 0 and go up continuously.  Besides the port
# information each node also has a variety of strings.  These strings
# are set by tb-* commands and dumped to the DB but are otherwise
# uninterpreted.
######################################################################

Class Node -superclass NSObject

Node instproc init {s} {
    $self set sim $s

    # portlist is a list of connections for the node.  It is sorted
    # by portnumber.  I.e. the ith element of portlist is the connection
    # on port i.
    $self set portlist {}

    # iplist, like portlist, is supported by portnumber.  An entry of
    # {} indicates an unassigned IP address for that port.
    $self set iplist {}

    # The type of the node.
    $self set type "pc" 

    # If osid remains blank when updatedb is called it is changed
    # to the default OS based on it's type (taken from node_types
    # table).
    $self set osid ""

    # These are just various strings that we pass through to the DB.
    $self set cmdline ""
    $self set rpms ""
    $self set startup ""
    $self set deltas ""
    $self set tarfiles ""
    $self set failureaction "fatal"
}

# The following procs support renaming (see README)
Node instproc rename {old new} {
    $self instvar portlist
    foreach object $portlist {
	$object rename_node $old $new
    }
    [$self set sim] rename_node $old $new
}

Node instproc rename_lanlink {old new} {
    $self instvar portlist
    set newportlist {}
    foreach node $portlist {
	if {$node == $old} {
	    lappend newportlist $new
	} else {
	    lappend newportlist $node
	}
    }
    set portlist $newportlist
}

# updatedb DB
# This adds a row to the virt_nodes table corresponding to this node.
Node instproc updatedb {DB} {
    $self instvar portlist
    $self instvar type
    $self instvar osid
    $self instvar cmdline
    $self instvar rpms
    $self instvar startup
    $self instvar deltas
    $self instvar iplist
    $self instvar tarfiles
    $self instvar failureaction
    var_import ::GLOBALS::pid
    var_import ::GLOBALS::eid

    # If we haven't specified a osid so far then we should fill it
    # with the id from the node_types table now.
    if {$osid == {}} {
	sql query $DB "select osid from node_types where type = \"$type\""
	set osid [sql fetchrow $DB]
	sql endquery $DB
    }

    # We need to generate the IP column from our iplist.
    set ipraw {}
    set i 0
    foreach ip $iplist {
	lappend ipraw $i:$ip
	incr i
    }

    # Update the DB
    sql exec $DB "insert into virt_nodes (pid,eid,vname,type,ips,osid,cmd_line,rpms,deltas,startupcmd,tarfiles,failureaction) values (\"$pid\",\"$eid\",\"$self\",\"$type\",\"$ipraw\",\"$osid\",\"$cmdline\",\"$rpms\",\"$deltas\",\"$startup\",\"$tarfiles\",\"$failureaction\")";
}

# add_lanlink lanlink
# This creates a new virtual port and connects the specified LanLink to it.
# The port number is returned.
Node instproc add_lanlink {lanlink} {
    $self instvar portlist
    $self instvar iplist
    lappend portlist $lanlink
    lappend iplist ""
    return [expr [llength $portlist] - 1]
}

# ip port
# ip port ip
# In the first form this returns the IP address associated with the port.
# In the second from this sets the IP address of a port.
Node instproc ip {port args} {
    $self instvar iplist
    $self instvar sim
    if {$args == {}} {
	return [lindex $iplist $port]
    } else {
	set ip [lindex $args 0]
	set subnet [join [lrange [split $ip .] 0 2] .]
	set iplist [lreplace $iplist $port $port $ip]
	$sim use_subnet $subnet
    }    
}

# find_port lanlink
# This takes a lanlink and returns the port it is connected to or 
# -1 if there is no connection.
Node instproc find_port {lanlink} {
    return [lsearch [$self set portlist] $lanlink]
}
