<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Send us an email message!");

echo "<p>
      What, you expected a fancy form? Maybe someday ...

      <p>
      For now, here is a plain old hyperlink: $TBMAILADDR
      \n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
