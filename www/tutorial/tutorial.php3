<?php
chdir("..");
require("defs.php3");

#
# Standard Testbed Header
#
if (!$printable) {
    PAGEHEADER("Emulab Tutorial");
}

if (!$printable) {
    echo("<b><a href=$REQUEST_URI?printable=1>Printable version of this document</a></b><br>");
}


chdir("tutorial");
readfile("tutorial.html");

#
# Standard Testbed Footer
# 
if (!$printable) {
    PAGEFOOTER();
}
?>

