#!/usr/bin/awk -f
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2004 University of Utah and the Flux Group.
# All rights reserved.
#

BEGIN {
    found = 0;
}

/^CPU:.*\((29[5-9][0-9]|30[0-4][0-9])\.[0-9]+\-MHz/ {
    print "3000";
    found = 1;
    exit
}
/^CPU:.*\((27[5-9][0-9]|28[0-4][0-9])\.[0-9]+\-MHz/ {
    print "2800";
    found = 1;
    exit
}
/^CPU:.*\((24[5-9][0-9]|25[0-4][0-9])\.[0-9]+\-MHz/ {
    print "2500";
    found = 1;
    exit
}
/^CPU:.*\((23[5-9][0-9]|24[0-4][0-9])\.[0-9]+\-MHz/ {
    print "2400";
    found = 1;
    exit
}
/^CPU:.*\((19[5-9][0-9]|20[0-4][0-9])\.[0-9]+\-MHz/ {
    print "2000";
    found = 1;
    exit
}
/^CPU:.*\((17[5-9][0-9]|18[0-4][0-9])\.[0-9]+\-MHz/ {
    print "1800";
    found = 1;
    exit
}
/^CPU:.*\((14[5-9][0-9]|15[0-4][0-9])\.[0-9]+\-MHz/ {
    print "1500";
    found = 1;
    exit
}
/^CPU:.*\(8[0-9][0-9]\.[0-9]+\-MHz/ {
    print "850";
    found = 1;
    exit
}
/^CPU:.*\((72[0-9]|73[0-9])\.[0-9]+\-MHz/ {
    print "733";
    found = 1;
    exit
}
/^CPU:.*\(6[0-9][0-9]\.[0-9]+\-MHz/ {
    print "600";
    found = 1;
    exit
}
/^CPU:.*\((29[0-9]|30[0-9]|333)\.[0-9]+\-MHz/ {
    print "300";
    found = 1;
    exit
}

END {
    if (found == 0) {
	print "0";
    }
}
