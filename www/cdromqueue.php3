<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Only known and logged in users can do this.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

if (!$isadmin) {
    USERERROR("You do not have permission to view this page!", 1);
}

if (isset($deletekey)) {
    #
    # Get the actual key.
    #
    $query_result =
	DBQueryFatal("select * from cdroms where cdkey='$deletekey'");

    if (! mysql_num_rows($query_result)) {
	USERERROR("No such CDROM request!", 1);
    }
    $row   = mysql_fetch_array($query_result);
    $name  = $row[user_name];
    $email = $row[user_email];
    $when  = $row[requested];

    #
    # We run this twice. The first time we are checking for a confirmation
    # by putting up a form. The next time through the confirmation will be
    # set. Or, the user can hit the cancel button, in which case we should
    # Probably redirect the browser back up a level.
    #
    if ($canceled) {
        PAGEHEADER("CDROM Deletion Request");
    
        echo "<center><h2><br>
              CDROM Request deletion has been canceled!
              </h2></center>\n";

        echo "<br>
              Back to <a href=cdromqueue.php3>CDROM request queue.</a>\n";
    
        PAGEFOOTER();
        return;
    }

    if (!$confirmed) {
        PAGEHEADER("CDROM Deletion Request");

	echo "<center><h2><br>
              Are you <b>REALLY</b> sure you want to delete this CDROM request?
              </h2>\n";

	echo "<form action=cdromqueue.php3 method=post>";
	echo "<input type=hidden name=deletekey value=$deletekey>\n";
	echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
	echo "<b><input type=submit name=canceled value=Cancel></b>\n";
	echo "</form>\n";
	echo "</center>\n";

	echo "<table align=center border=1 cellpadding=2 cellspacing=2>\n";
	echo "<tr>
  	          <td>$name</td>
  	          <td>$email</td>
	          <td>$when</td>
             </tr>\n";
	echo "</table>\n";

	PAGEFOOTER();
	return;
    }
    DBQueryFatal("delete from cdroms where cdkey='$deletekey'");
    header("Location: cdromqueue.php3");
}

#
# Get the list and show it.
#
#
# Standard Testbed Header, now that we know what we want to say.
#
PAGEHEADER("CDROM Request Queue");

$query_result =
    DBQueryFatal("select * from cdroms order by requested DESC");

if (! mysql_num_rows($query_result)) {
    USERERROR("There are no requests in the queue!\n", 1);
}

echo "<table align=center border=1 cellpadding=2 cellspacing=2>\n";

echo "<tr>
          <td>Delete?</td>
          <td>Name</td>
          <td>Email</td>
          <td>Key</td>
          <td>When</td>
      </tr>\n";

while ($row = mysql_fetch_array($query_result)) {
    $name  = $row[user_name];
    $email = $row[user_email];
    $when  = $row[requested];
    $cdkey = $row[cdkey];

    echo "<tr>
              <td align=center>
                  <A href='cdromqueue.php3?deletekey=$cdkey'>
	                  <img alt=X src=redball.gif></A>
	      <td>$name</td>
	      <td>$email</td>
	      <td>$cdkey</td>
	      <td>$when</td>
         </tr>\n";
}
echo "</table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
