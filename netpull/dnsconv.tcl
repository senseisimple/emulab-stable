
while {[gets stdin line] >= 0} {
    if {$line == "" || [string index $line 0] == "#"} {continue}
    set fp [open "|host $line" r]
    set ip $line
    while {[gets $fp hline] >= 0} {
	if {[regexp {address ([0-9\.]+)} $hline match addr]} {
	    set ip $addr
	}
    }
    puts $ip
    close $fp
}