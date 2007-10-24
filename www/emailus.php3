<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Send us an email message!");

echo "<p>
      What, you expected a fancy form? 
      <p>
      Sorry, here is a plain old hyperlink: $TBMAILADDR
      \n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
