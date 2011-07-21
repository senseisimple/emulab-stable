#!/usr/bin/awk -f
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#

BEGIN {
    str="";
}
/^pchb0.*"Intel [0-9][0-9][0-9][0-9][0-9]BX"/ {
    str="BX";
    exit 0
}
/^pchb0.*"Intel [0-9][0-9][0-9][0-9][0-9]GX"/ {
    str="GX";
    exit 0
}
END {
    print str;
    exit 0
}
