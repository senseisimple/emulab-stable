<?php
#
# Beware empty spaces (cookies)!
# 
require("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Non Existent Page!");

USERERROR("The URL you gave: <b>$REQUEST_URI</b>
           is not available or is broken.", 1);

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

