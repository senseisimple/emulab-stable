<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Only known and logged in users can do this.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

if (!$isadmin) {
    USERERROR("You do not have permission to view this page!", 1);
}

if (isset($deletekey)) {
    #
    # Get the actual key.
    #
    $query_result =
	DBQueryFatal("select * from widearea_privkeys where idx=$deletekey");

    if (! mysql_num_rows($query_result)) {
	USERERROR("No such CDROM request!", 1);
    }
    $row      = mysql_fetch_array($query_result);
    $privkey  = $row[privkey];
    $cdidx    = $row[cdidx];
    $imageidx = $row[imageidx];
    $updated  = $row[updated];
    $IP       = $row[IP];

    #
    # We run this twice. The first time we are checking for a confirmation
    # by putting up a form. The next time through the confirmation will be
    # set. Or, the user can hit the cancel button, in which case we should
    # Probably redirect the browser back up a level.
    #
    if ($canceled) {
        PAGEHEADER("Widearea Key Deletion Request");
    
        echo "<center><h2><br>
              Widearea Key deletion has been canceled!
              </h2></center>\n";

        echo "<br>
              Back to <a href=wideareakeys.php3>CDROM request queue.</a>\n";
    
        PAGEFOOTER();
        return;
    }

    if (!$confirmed) {
        PAGEHEADER("Widearea Key Deletion Request");

	echo "<center><h2><br>
              Are you <b>REALLY</b> sure you want to delete this Widearea Key?
              </h2>\n";

	echo "<form action=wideareakeys.php3 method=post>";
	echo "<input type=hidden name=deletekey value=$deletekey>\n";
	echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
	echo "<b><input type=submit name=canceled value=Cancel></b>\n";
	echo "</form>\n";
	echo "</center>\n";

	echo "<table align=center border=1 cellpadding=2 cellspacing=2>\n";
	echo "<tr>
	          <td>$IP</td>
                  <td>$privkey</td>
  	          <td>$cdidx</td>
  	          <td>$imageidx</td>
	          <td>$updated</td>
             </tr>\n";
	echo "</table>\n";

	PAGEFOOTER();
	return;
    }
    DBQueryFatal("delete from widearea_privkeys where idx=$deletekey");
    header("Location: wideareakeys.php3");
}

#
# Get the list and show it.
#
#
# Standard Testbed Header, now that we know what we want to say.
#
PAGEHEADER("Widearea Private Keys");

$query_result =
    DBQueryFatal("select * from widearea_privkeys order by updated DESC");

if (! mysql_num_rows($query_result)) {
    USERERROR("There are no widearea keys!\n", 1);
}

echo "<table align=center border=1 cellpadding=2 cellspacing=2>\n";

echo "<tr>
          <td>Delete?</td>
          <td>IP</td>
          <td>privkey</td>
          <td>cd</td>
          <td>image</td>
          <td>Updated</td>
      </tr>\n";

while ($row = mysql_fetch_array($query_result)) {
    $idx      = $row[idx];
    $privkey  = $row[privkey];
    $cdidx    = $row[cdidx];
    $imageidx = $row[imageidx];
    $updated  = $row[updated];
    $IP       = $row[IP];

    echo "<tr>
              <td align=center>
                  <A href='wideareakeys.php3?deletekey=$idx'>
	                  <img alt=X src=redball.gif></A>
	      <td>$IP</td>
	      <td>$privkey</td>
	      <td>$cdidx</td>
	      <td>$imageidx</td>
	      <td>$updated</td>
         </tr>\n";
}
echo "</table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
