Class lan

# print

# output format: <id> "<nodes>" <delay> <bw>
lan instproc print {file} {
    global nodeid_map prefix

    if {[info exists nodeid_map(lan[$self set id])]} {
	set lanname $nodeid_map(lan[$self set id])
    } else {
	set lanname lan[$self set id]
    }

    puts -nonewline $file "$prefix-$lanname \""
    set lastnode [lindex [$self set nodes] end]
    foreach node [$self set nodes] {
	if {[info exists nodeid_map(n[$node set id])]} {
	    set nodename $nodeid_map(n[$node set id])
	} else {
	    set nodename n[$self set id]
	}
	puts -nonewline $file "$nodename"
	if {$node != $lastnode} {
	    puts -nonewline $file " "
	}
    }
    puts -nonewline $file "\" [$self set bw] [$self set delay] "
    $self instvar lanlinks
    if {! [info exists lanlinks]} {
	set lanlinks {}
    }
    foreach link [$self set lanlinks] {
	puts -nonewline $file "$prefix-$link "
    }
    puts $file ""
}

#add link

lan instproc addlink {link} {
$self instvar lanlinks
lappend lanlinks $link 
}