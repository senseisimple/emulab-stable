Class link 

#output format is 
# link src srcport dst dstport

link instproc print {file} {
    global nodeid_map prefix
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
    
    puts $file "$prefix-l[$self set id] $srcname [$self set srcport] $dstname [$self set dstport] [$self set bw] [$self set bw] [$self set delay] [$self set delay]"
}

