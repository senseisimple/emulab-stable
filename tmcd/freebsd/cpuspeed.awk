#!/usr/bin/awk -f
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2004 University of Utah and the Flux Group.
# All rights reserved.
#

/^CPU:.*\((24[5-9][0-9]|25[0-4][0-9])\.[0-9]+\-MHz/ {
    print "2500";
    exit
}
/^CPU:.*\((23[5-9][0-9]|24[0-4][0-9])\.[0-9]+\-MHz/ {
    print "2400";
    exit
}
/^CPU:.*\((19[5-9][0-9]|20[0-4][0-9])\.[0-9]+\-MHz/ {
    print "2000";
    exit
}
/^CPU:.*\((17[5-9][0-9]|18[0-4][0-9])\.[0-9]+\-MHz/ {
    print "1800";
    exit
}
/^CPU:.*\((14[5-9][0-9]|15[0-4][0-9])\.[0-9]+\-MHz/ {
    print "1500";
    exit
}
/^CPU:.*\(8[0-9][0-9]\.[0-9]+\-MHz/ {
    print "850";
    exit
}
/^CPU:.*\((72[0-9]|73[0-9])\.[0-9]+\-MHz/ {
    print "733";
    exit
}
/^CPU:.*\(6[0-9][0-9]\.[0-9]+\-MHz/ {
    print "600";
    exit
}
/^CPU:.*\(29[0-9]|30[0-9]\.[0-9]+\-MHz/ {
    print "300";
    exit
}
/^CPU:.*/ {
    print "0";
    exit
}
