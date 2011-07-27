#!/usr/bin/awk -f
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2004 University of Utah and the Flux Group.
# All rights reserved.
#

/^cpu MHz.*(29[5-9][0-9]|30[0-4][0-9])\.[0-9]+$/ {
    print "3000";
    exit
}
/^cpu MHz.*(27[5-9][0-9]|28[0-4][0-9])\.[0-9]+$/ {
    print "2800";
    exit
}
/^cpu MHz.*(24[5-9][0-9]|25[0-4][0-9])\.[0-9]+$/ {
    print "2500";
    exit
}
/^cpu MHz.*(23[5-9][0-9]|24[0-4][0-9])\.[0-9]+$/ {
    print "2400";
    exit
}
/^cpu MHz.*(19[5-9][0-9]|20[0-4][0-9])\.[0-9]+$/ {
    print "2000";
    exit
}
/^cpu MHz.*(14[5-9][0-9]|15[0-4][0-9])\.[0-9]+$/ {
    print "1500";
    exit
}
/^cpu MHz.*8[0-9][0-9]\.[0-9]+$/ {
    print "850";
    exit
}
/^cpu MHz.*(72[0-9]|73[0-9])\.[0-9]+$/ {
    print "733";
    exit
}
/^cpu MHz.*6[0-9][0-9]\.[0-9]+$/ {
    print "600";
    exit
}
/^cpu MHz.*(29[0-9]|30[0-9])\.[0-9]+$/ {
    print "300";
    exit
}
/^cpu MHz.*/ {
    print "0";
    exit
}
