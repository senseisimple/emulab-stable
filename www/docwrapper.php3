<?php
require("defs.php3");

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

#
# Want want to spit out a basepath tag since that mimics typical behaviour
# wrt relative links in the doc, when the target file is in a subdir.
#
if (strrchr($docname, "/")) {
    $dir     = substr($docname, 0, strrpos($docname, "/"));
    $docname = substr(strrchr($docname, "/"), 1);
    if (isset($SSL_PROTOCOL)) {
	echo "<base href='$TBBASE/$dir/'>\n";
    }
    else {
	echo "<base href='$TBDOCBASE/$dir/'>\n";
    }
    chdir($dir);
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

