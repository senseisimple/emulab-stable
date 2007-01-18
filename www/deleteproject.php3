<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003, 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Terminating Project and Remove all Trace");

#
# Only known and logged in users can end experiments.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Currently, only admin users can do this. Change later.
#
if (! $isadmin) {
    USERERROR("You do not have permission to remove project '$pid'", 1);
}

#
# Confirm a real project
#
if (! TBValidProject($pid)) {
    USERERROR("No such project '$pid'", 1);
}

#
# Check to see if there are any active experiments. Abort if there are.
#
$query_result =
    DBQueryFatal("SELECT * FROM experiments where pid='$pid'");
if (mysql_num_rows($query_result)) {
    USERERROR("Project '$pid' has active experiments. You must terminate ".
	      "those experiments before you can remove the project!", 1);
}

#
# We run this twice. The first time we are checking for a confirmation
# by putting up a form. The next time through the confirmation will be
# set. Or, the user can hit the cancel button, in which case we should
# probably redirect the browser back up a level.
#
if ($canceled) {
    echo "<center><h2>
          Project removal canceled!
          </h2></center>\n";
    
    PAGEFOOTER();
    return;
}

if (!$confirmed) {
    echo "<center><h2>
          Are you <b>REALLY</b> sure you want to remove Project '$pid?'
          </h2>\n";
    
    echo "<form action=\"deleteproject.php3\" method=\"post\">";
    echo "<input type=hidden name=pid value=\"$pid\">\n";
    echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
    echo "</form>\n";
    echo "</center>\n";

    PAGEFOOTER();
    return;
}

if (!$confirmed_twice) {
    echo "<center><h2>
	  Okay, lets be sure.<br>
          Are you <b>REALLY REALLY</b> sure you want to remove Project '$pid?'
          </h2>\n";
    
    echo "<form action=\"deleteproject.php3\" method=\"post\">";
    echo "<input type=hidden name=pid value=\"$pid\">\n";
    echo "<input type=hidden name=confirmed value=Confirm>\n";
    echo "<b><input type=submit name=confirmed_twice value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
    echo "</form>\n";
    echo "</center>\n";

    PAGEFOOTER();
    return;
}

echo "<br>
      Project '$pid' is being removed!<br><br>
      This will take a minute or two. <b>Please</b> do not click the Stop
      button during this time. If you do not receive notification within
      a reasonable amount of time, please contact $TBMAILADDR.<br>\n";
flush();

#
# Remove the project directory and the group.
#
SUEXEC($uid, $TBADMINGROUP, "webrmproj $pid", 1);

#
# Warm fuzzies.
#
echo "<br>
      <b>Done!</b>
      <br>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
