#!/usr/local/bin/tclsh

# given a config file for switch and vlan configuration file,
# modify switch config file to contain new vlan information.
# currently, switch config file format is detailed in ~place/vlan/format-iq.

# vlan names must already be known to switch

# config file:
#
# format is
# vlan_name
# port_number port_number port_number port_number ... (all on one line)
# vlan_name
# port_number port_number port_number port_number ...
# (and so on)

if {$argc != 2} {
   error "two arguments, _iqadfile_ and _configfile_\n"
}

set iqadfile [open [lindex $argv 0] r+]
set vlanfile [open [lindex $argv 1] r]

set iqadcontents [read $iqadfile]
set DSA [string first "STDALONE" $iqadcontents]
# DSA == Domain String Address. everything I want is at a fixed offset from
# this string

seek $iqadfile [expr $DSA + 0x22]

while {[gets $vlanfile line] >=0} {
   # vlan name
   set line [string trimleft $line]
   puts -nonewline $iqadfile $line
   puts -nonewline $iqadfile [format "%c" 0]
   #field length 0x14 bytes (-1 for the \0 byte)
   seek $iqadfile [expr 0x13-[string length $line]]  current

   # port list
   gets $vlanfile line
   set ports [split $line]
   #every port has a magic unique number 
   set magicnumber 0xc9
   foreach port $ports {
      scan $port "%d" portnum
      set portstr [format "%c%c" $magicnumber $portnum] 
      #one byte for magic number and one for port
      puts -nonewline $iqadfile $portstr
      incr magicnumber
   }
   lappend vlanEndOffsets [expr [tell $iqadfile] -$DSA -0x1A]
}

#write config length.
set configlen [expr [tell $iqadfile] - $DSA - 0x1A]
seek $iqadfile [expr $DSA + 0x1F]
puts -nonewline $iqadfile [format "%c" $configlen]

#write end-of-vlan offsets
seek $iqadfile [expr $DSA + 0xFEF]
foreach offset $vlanEndOffsets {
   set offsetstr [format "%c%c" $offset 0]
# null byte follows each offset
   puts -nonewline $iqadfile $offsetstr
}
close $iqadfile
close $vlanfile
