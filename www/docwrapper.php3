<?php
require("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Emulab Documentation");

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

readfile("$docname");

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

