Class node

#print

# output format is: nodeID type link(s)
node instproc print {file} {
    global nodeid_map
    global prefix

    if {[info exists nodeid_map(n[$self set id])]} {
	set nodename $nodeid_map(n[$self set id])
    } else {
	set nodename n[$self set id]
    }
    $self instvar nodelinks
    if {! [info exists nodelinks]} {
	set nodelinks {}
    }
    puts -nonewline $file "$nodename [$self set type]"
    foreach link $nodelinks {
	puts -nonewline $file " $prefix-$link"
    }
    #we have to add ". bandwidth delay" to delay nodes.
    if [string match [$self set type] delay] {
	puts -nonewline $file " . [$self set bw] [$self set delay]"
    }
    
    puts $file ""  
}


#add link

node instproc addlink {link} {
$self instvar nodelinks
lappend nodelinks $link 
}

#getLan/setLan

node instproc setLan {lan} {
    $self set lan $lan
}

node instproc getLan {} {
    $self instvar lan
    if {![info exists lan]} {
	$self set lan ""
    }
    return [$self set lan]
}