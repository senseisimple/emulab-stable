<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Request a New Emulab Account");

echo "<center><font size=+1>
       If you already have an Emulab account, <a href=login.php3>
       please log on first!</a>
       <br><br>
       <a href=joinproject.php3>Join an Existing Project</a>.
       <br>
       or
       <br>
       <a href=newproject.php3>Start a New Project</a>.
       <br>
       <font size=-1>
       If you are a <font color=red>student (undergrad or graduate)</font>,
       please do not try to start a project!<br> Your advisor must do it.
       </font>
      </font></center><br>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
