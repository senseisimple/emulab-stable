#!/usr/bin/awk -f
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#

#
# "true" pc850 reports:
#    Host bridge: Intel Corp. 440BX/ZX/DX - 82443BX/ZX/DX Host bridge (AGP disabled) (rev 3).
# upgraded pc600 reports:
#    Host bridge: Intel Corp. 440BX/ZX/DX - 82443BX/ZX/DX Host bridge (rev 3).
#
/^[ ]+Host bridge: Intel Corp.*[0-9][0-9][0-9][0-9][0-9]BX.*\(AGP disabled\)/ {
    print "BX";
    exit
}
/^[ ]+Host bridge: Intel Corp.*[0-9][0-9][0-9][0-9][0-9]BX/ {
    print "BX-AGP";
    exit
}
/^[ ]+Host bridge: Intel Corp.*[0-9][0-9][0-9][0-9][0-9]GX/ {
    print "GX";
    exit
}
/^[ ]+Host bridge:.*/ {
    print "??";
    exit
}
