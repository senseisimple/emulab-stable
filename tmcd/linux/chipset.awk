#!/usr/bin/awk -f
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#

/^[ ]+Host bridge: Intel Corporation.*[0-9][0-9][0-9][0-9][0-9]BX/ {
    print "BX";
    next
}
/^[ ]+Host bridge: Intel Corporation.*[0-9][0-9][0-9][0-9][0-9]GX/ {
    print "GX";
    next
}
/^[ ]+Host bridge:.*/ {
    print "??";
    next
}
