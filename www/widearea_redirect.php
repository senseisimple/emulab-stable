<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#
require("defs.php3");

#
# Return info about the widearea project. Ignore the IP arg for now.
# Anyone can run this page. No login is needed.
# 
PAGEHEADER("Widearea Node Info");

echo "You have been redirected to this page by a host this is part of
      www.netbed.org's <a href=cdrom.php>widearea experiment.</a>
      <br>
      <br>
      If you have problems with this host or questions
      about its configuration, please contact $TBMAILADDR.\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
