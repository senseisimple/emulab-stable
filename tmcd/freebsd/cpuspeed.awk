#!/usr/bin/awk -f

/^CPU:.*\(8[0-9][0-9]\.[0-9]+\-MHz/ {
    print "850";
    next
}
/^CPU:.*\(6[0-9][0-9]\.[0-9]+\-MHz/ {
    print "600";
    next
}
/^CPU:.*/ {
    print "0";
    next
}
