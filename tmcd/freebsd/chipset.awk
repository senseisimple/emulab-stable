#!/usr/bin/awk -f

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
