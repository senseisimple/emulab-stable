#!/usr/local/bin/tclsh
# This program takes a ns file as input and produces the TB commands as output.
# The TB commands are printed without the common prefix #TB

while {[gets stdin line] >= 0} {
    if {[lindex $line 0] == "\#TB"} {
	puts [string range $line 4 end]
    }
}
