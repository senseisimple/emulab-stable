#!/usr/bin/awk -f
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#

#
# "true" pc850 reports:
#    pcib0: <Intel 82443BX host to PCI bridge (AGP disabled)> on motherboard
# upgraded pc600 reports:
#    pcib0: <Intel 82443BX (440 BX) host to PCI bridge> on motherboard
#
/^pcib0: <Intel [0-9][0-9][0-9][0-9][0-9]BX .*(AGP disabled)>/ {
    print "BX";
    exit
}
/^pcib0: <Intel [0-9][0-9][0-9][0-9][0-9]BX / {
    print "BX-AGP";
    exit
}
/^pcib0: <Intel [0-9][0-9][0-9][0-9][0-9]GX / {
    print "GX";
    exit
}
/^pcib0:.*/ {
    print "??";
    exit
}
