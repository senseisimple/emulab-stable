Class link 

#output format is 
# link src srcport dst dstport

link instproc print {file} {
    global nodeid_map prefix lanlist
    if {[info exists nodeid_map([$self set src])]} {
	set srcname $nodeid_map([$self set src])
    } else {
	set srcname [$self set src]
    }
    if {[info exists nodeid_map([$self set dst])]} {
	set dstname $nodeid_map([$self set dst])
    } else {
	set dstname [$self set dst]
    }
    if {[lsearch $lanlist [$self set src]] != -1} {
	set srcname $prefix-$srcname
    }
    if {[lsearch $lanlist [$self set dst]] != -1} {
	set dstname $prefix-$dstname
    }
    if {[info exists nodeid_map(l[$self set id])]} {
	set linkname $nodeid_map(l[$self set id])
    } else {
	set linkname "l[$self set id]"
    }
    puts $file "$prefix-$linkname $srcname [$self set srcport] $dstname [$self set dstport] [$self set bw] [$self set bw] [$self set delay] [$self set delay]"
}

