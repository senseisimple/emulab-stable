#!/usr/bin/awk -f
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2004 University of Utah and the Flux Group.
# All rights reserved.
#

BEGIN {
    found = 0;
}

#
# "true" pc850 reports:
#    pcib0: <Intel 82443BX host to PCI bridge (AGP disabled)> on motherboard
# upgraded pc600 reports:
#    pcib0: <Intel 82443BX (440 BX) host to PCI bridge> on motherboard
#
/^pcib0: <Intel [0-9][0-9][0-9][0-9][0-9]BX host.*\(AGP disabled\)/ {
    print "BX";
    found = 1;
    exit
}
/^pcib0: <Intel [0-9][0-9][0-9][0-9][0-9]BX \(440 BX\) host/ {
    print "BX-AGP";
    found = 1;
    exit
}
/^pcib0: <Intel [0-9][0-9][0-9][0-9][0-9]GX / {
    print "GX";
    found = 1;
    exit
}

#
# pc850 under FreeBSD 5.x
#
/^acpi0: <INTEL  TR440BXA> on motherboard/ {
    print "BX";
    found = 1;
    exit
}

#
# aero:    pcib1: <PCI to PCI bridge (vendor=8086 device=2545)> ... Intel HI_C
# rutgers: pcib1: <PCI to PCI bridge (vendor=8086 device=2543)> ... Intel HI_B
#
/^pcib1: <PCI to PCI bridge \(vendor=8086 device=2545\)>/ {
    print "HI_C";
    found = 1;
    exit
}
/^pcib1: <PCI to PCI bridge \(vendor=8086 device=2543\)>/ {
    print "HI_B";
    found = 1;
    exit
}

END {
    if (found == 0) {
	print "??";
    }
}
