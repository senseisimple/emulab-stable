#!/usr/bin/awk -f

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
