<?php
chdir("..");
require("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Emulab Tutorial");

echo("<b><a href=$TBDOCBASE/docwrapper.php3?docname=tutorial/tutorial.html&printable=1>Printable version of this document</a></b><br>");

chdir("tutorial");
readfile("tutorial.html");

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

