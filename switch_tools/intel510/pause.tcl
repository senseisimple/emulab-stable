#!/usr/local/bin/tclsh
#
# Usage:
#
# pause <ms>
#
# Pauses for <ms> milliseconds.

set ms [lindex $argv 0]
if {! [regexp {[0-9]+} $ms]} {
    puts stderr "Usage: $argv0 <ms>"
    puts stderr "Pauses for <ms> milliseconds."
    exit 1
} else {
    after $ms
}
