#!/usr/bin/awk -f

/^cpu MHz.*8[0-9][0-9]\.[0-9]+$/ {
    print "850";
    next
}
/^cpu MHz.*6[0-9][0-9]\.[0-9]+$/ {
    print "600";
    next
}
/^cpu MHz.*/ {
    print "0";
    next
}
