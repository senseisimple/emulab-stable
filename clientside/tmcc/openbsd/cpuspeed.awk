#!/usr/bin/awk -f
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#

/^cpu0:.*\) (19|20)[0-9][0-9] MHz/ {
    print "2000";
    next
}
/^cpu0:.*\) 1[45][0-9][0-9] MHz/ {
    print "1500";
    next
}
/^cpu0:.*\) 8[0-9][0-9] MHz/ {
    print "850";
    next
}
/^cpu0:.*\) 6[0-9][0-9] MHz/ {
    print "600";
    next
}
/^cpu0:.*MHz/ {
    print "0";
    next
}
