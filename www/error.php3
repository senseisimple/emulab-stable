<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
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

