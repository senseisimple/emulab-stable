#!/usr/bin/awk -f

/^CPU:.*\(1[45][0-9][0-9]\.[0-9]+\-MHz/ {
    print "1500";
    next
}
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
