#!/usr/bin/awk -f
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#

/^pcib0: <Intel [0-9][0-9][0-9][0-9][0-9]BX / {
    print "BX";
    next
}
/^pcib0: <Intel [0-9][0-9][0-9][0-9][0-9]GX / {
    print "GX";
    next
}
/^pcib0:.*/ {
    print "??";
    next
}
