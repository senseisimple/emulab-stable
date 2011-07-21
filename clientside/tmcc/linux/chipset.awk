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
#    Host bridge: Intel Corp. 440BX/ZX/DX - 82443BX/ZX/DX Host bridge (AGP disabled) (rev 3).
# upgraded pc600 reports:
#    Host bridge: Intel Corp. 440BX/ZX/DX - 82443BX/ZX/DX Host bridge (rev 3).
#

/^[ ]+Host bridge: Intel Corp.*[0-9][0-9][0-9][0-9][0-9]BX.*\(AGP disabled\)/ {
    print "BX";
    found = 1;
    exit
}
/^[ ]+Host bridge: Intel Corp.*[0-9][0-9][0-9][0-9][0-9]BX/ {
    print "BX-AGP";
    found = 1;
    exit
}
/^[ ]+Host bridge: Intel Corp.*[0-9][0-9][0-9][0-9][0-9]GX/ {
    print "GX";
    found = 1;
    exit
}
/^[ ]+PCI bridge: Intel Corp.*HI_C Virtual PCI-to-PCI Bridge/ {
    print "HI_C";
    found = 1;
    exit
}
/^[ ]+PCI bridge: Intel Corp.*HI_B Virtual PCI-to-PCI Bridge/ {
    print "HI_B";
    found = 1;
    exit
}
END {
    if (found == 0) {
	print "??";
    }
}
