#!/usr/local/bin/tclsh

# given the testbed intermediate representation (IR) on stdin,
# modify switch config file to contain new vlan information.
# currently, switch config file format is detailed in ~place/vlan/format-iq.

# vlan names must already be known to switch

proc config_switch name {
   global argv0
   switch $name {
      alpha {set filename iqadbad4.p}
      beta  {set filename iqad24c3.p}
      default {
         puts stderr "$argv0 : switch name $name is not valid"
         exit 1
      }
   }

   if {[catch "open $filename r+" iqadfile]} {
      puts stderr "$argv0 $errorInfo"
      exit 1
   }

   # switch_name to stdout
   puts -nonewline "$name VLANs:"

   set iqadcontents [read $iqadfile]
   set DSA [string first "STDALONE" $iqadcontents]
   # DSA == Domain String Address. everything I want is at a fixed offset from
   # this string

   # clear any existing VLAN info
   seek $iqadfile [expr $DSA + 0x22]
   for {set i 0} {$i < 0x1f3} {incr i} {
      puts -nonewline $iqadfile "\0\0\0\0\0\0\0\0"
   }

   seek $iqadfile [expr $DSA + 0x22]

   # for a switch, each line goes as follows:
   # vlan_name port port port port port ...
   while {[gets stdin line] >=0} {
      # comment found
      if {[regexp -nocase {^[ \t]*#} $line]} {continue}
      #empty line
      if {[regexp -nocase {^[ \t]*$} $line]} {continue}

      # "end" of switch config 
      if {[regexp -nocase {^[ \t]*end[ \t]*$} $line]} {break}

      # vlan name is first field
      set line [split [string trim $line]]

      # write NULL-terminated VLAN name
      set vlanname [lindex $line 0]
      puts -nonewline " $vlanname"
      puts -nonewline $iqadfile $vlanname
      puts -nonewline $iqadfile [format "%c" 0]
      #field length 0x14 bytes (-1 for the \0 byte)
      seek $iqadfile [expr 0x13-[string length $vlanname]]  current

      #every port has a magic unique number.
      #they start at 0xc9 for no discernible reason 
      set magicnumber 0xc9

      # port list
      set listsize [llength $line]
      for {set i 1} {$i < $listsize} {incr i} {
         set port [lindex $line $i]
         # split may have given ""s when encountering whitespace
         if {$port != ""} {
            lappend ports $port
         }
      }
      set ports [lsort -integer $ports]
      set listsize [llength $ports]
      for {set i 0} {$i < $listsize} {incr i} {
         set curr [lindex $ports $i]
         if {$curr == ""} {continue}

         scan $curr "%d" portnum
         #one byte for magic number and one for port
         puts -nonewline $iqadfile [format "%c%c" $magicnumber $portnum]
         incr magicnumber
      }
      unset ports
      lappend vlanEndOffsets [expr [tell $iqadfile] -$DSA -0x1A]
   }

   if {[eof stdin]} {
      error "unexpected EOF in IR during VLAN processing"
      exit 1
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
   puts ""
}


# MAIN

puts "VLAN configuration\n Switches configured:"

while {[gets stdin line] >=0} {
   # comment
   if {[regexp -nocase {^[ \t]*#} $line]} {continue}
   # empty line
   if {[regexp -nocase {^[ \t]*$} $line]} {continue}

   # look for "START vlan"
   if {[regexp -nocase {^[ \t]*START[ \t]*vlan[ \t]*$} $line]} {

      # look for switches. format is "switch switchname"
      while {[gets stdin line] >=0} {
	 
         # ignore comments
         if {[string index $line 0] == "#"} {continue}

         # exit cleanly if end of VLAN config section reached
         if {[regexp -nocase {^[ \t]*END[ \t]*vlan[ \t]*$} $line]} {exit 0}

         if {[regexp -nocase {^[ \t]*switch.*} $line]} {
            #found switch line (format is "switch switch_name")
            #isolate the switch name
            set switchname [string trim [string range $line \
                [expr [string first "switch" [string tolower $line]] +7] end]]
            string trim $switchname
            config_switch $switchname
         }
      }
      # END vlan never encountered
      error "unexpected EOF in IR during VLAN processing"
      exit 1
   }
}
