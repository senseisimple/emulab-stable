Class link 

#output format is 
# link src srcport dst dstport

link instproc print {file} {
    global nodeid_map prefix lanlist
    if {[info exists nodeid_map([$self set src])]} {
	real_set srcname $nodeid_map([$self set src])
    } else {
	real_set srcname [$self set src]
    }
    if {[info exists nodeid_map([$self set dst])]} {
	real_set dstname $nodeid_map([$self set dst])
    } else {
	real_set dstname [$self set dst]
    }
    if {[lsearch $lanlist [$self set src]] != -1} {
	real_set srcname $prefix-$srcname
    }
    if {[lsearch $lanlist [$self set dst]] != -1} {
	real_set dstname $prefix-$dstname
    }
    if {[info exists nodeid_map(l[$self set id])]} {
	real_set linkname $nodeid_map(l[$self set id])
    } else {
	real_set linkname "l[$self set id]"
    }
    puts $file "$prefix-$linkname $srcname [$self set srcport] $dstname [$self set dstport] [$self set bw] [$self set bw] [$self set delay] [$self set delay]"
}

