<?php
chdir("..");
require("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Emulab Tutorial");

chdir("tutorial");
readfile("tutorial.html");

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

