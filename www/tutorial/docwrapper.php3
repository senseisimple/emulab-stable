<?php
chdir("..");
require("defs.php3");
chdir("tutorial");

#
# Standard Testbed Header
#
if (!$printable) {
    PAGEHEADER("Emulab Documentation");
}

#
# Need to sanity check the path! For now, just make sure the path
# does not start with a dot or a slash.
#
$first = substr($docname, 0, 1);
if (strcmp($first, ".") == 0 ||
    strcmp($first, "/") == 0) {
    USERERROR("Invalid document name: $docname!");
}
#
# Nothing that looks like a ../ is allowed anywhere in the name
#
if (strstr($docname, "../")) {
    USERERROR("Invalid document name: $docname!");
}

if (!$printable) {
	echo "<b><a href=$REQUEST_URI&printable=1>
                 Printable version of this document</a></b><br>";
}

readfile("$docname");

#
# Standard Testbed Footer
# 
if (!$printable) {
    PAGEFOOTER();
}
?>

