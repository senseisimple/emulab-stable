<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("User List");

#
#
# Only known and logged in users allowed.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Admin users can see all users, while normal users can only see
# users in their projects.
#
$isadmin = ISADMIN($uid);

#
# Get the project list.
#
if (! $isadmin) {
    USERERROR("You do not have permission to view the user list!", 1);
}

$query_result =
    DBQueryFatal("SELECT u.* FROM users as u order by u.uid");

if (mysql_num_rows($query_result) == 0) {
    USERERROR("There are no users!", 1);
}

#
# Grab the login info. If we cannot get the login info, then just
# proceed without it. 
#
$loginarray = LASTUSERSLOGIN(0);

echo "<table width=\"100%\" border=2 cellpadding=1 cellspacing=0
       align='center'>\n";

echo "<tr>
          <td>&nbsp</td>
          <td>UID</td>
          <td>Name</td>
          <td>Projects</td>
          <td>Web<br>Idle</td>
          <td>Users<br>Idle</td>\n";

#
# Admin users get a "delete" and a "modify" option.
# 
if ($isadmin) {
    echo "<td align=center>Modify</td>\n";
    echo "<td align=center>Delete</td>\n";
}
echo "</tr>\n";

while ($row = mysql_fetch_array($query_result)) {
    $thisuid  = $row[uid];
    $name     = $row[usr_name];
    $status   = $row[status];
    $unix_uid = $row[unix_uid];
    $lastweblogin   = LASTWEBLOGIN($thisuid);

    #
    # Suck out a list of projects too.
    #
    $projmemb_result = mysql_db_query($TBDBNAME,
	"SELECT distinct pid FROM group_membership ".
	"where uid='$thisuid' order by pid");

    echo "<tr>\n";

    if (strcmp($status, "active") == 0 ||
	strcmp($status, "unverified") == 0) {
	echo "<td align=center><img alt=\"Y\" src=\"greenball.gif\"></td>\n";
    }
    else {
	echo "<td align=center><img alt=\"N\" src=\"redball.gif\"></td>\n";
    }

    echo "<td><A href='showuser.php3?target_uid=$thisuid'>$thisuid</A></td>
              <td>$name</td>\n";

    if ($count = mysql_num_rows($projmemb_result)) {
	    echo "<td> ";
	    while ($projrow = mysql_fetch_array($projmemb_result)) {
		    $pid = $projrow[pid];
		    echo "<A href='showproject.php3?pid=$pid'>$pid</A>";
		    $count--;
		    if ($count)
			    echo ", ";
	    }
	    echo "</td>\n";
    }
    else {
	    echo "<td>--</td>\n";
    }

    #
    # Sleazy! Use mysql query to convert dates to days and subtract!
    #
    if ($lastweblogin) {
	$idle_query = mysql_db_query($TBDBNAME,
		"SELECT TO_DAYS(CURDATE()) - TO_DAYS(\"$lastweblogin\")");
	$idle_row   = mysql_fetch_row($idle_query);
	echo "<td>$idle_row[0]</td>\n";
    }
    else {
	echo "<td>N/A</td>\n";
    }

    if ($loginarray && isset($loginarray["$unix_uid"])) {
	$userslogininfo = $loginarray["$unix_uid"];
	$lastuserslogin = $userslogininfo["date"];

	$idle_query = mysql_db_query($TBDBNAME,
		"SELECT TO_DAYS(CURDATE()) - TO_DAYS(\"$lastuserslogin\")");
	$idle_row   = mysql_fetch_row($idle_query);
	echo "<td>$idle_row[0]</td>\n";
    }
    else {
	echo "<td>N/A</td>\n";
    }

    if ($isadmin) {
	echo "<td align=center><A href='modusr_form.php3?target_uid=$thisuid'>
                     <img alt=\"O\" src=\"blueball.gif\"></A></td>\n";
	echo "<td align=center><A href='deleteuser.php3?target_uid=$thisuid'>
                     <img alt=\"X\" src=\"redball.gif\"></A></td>\n";
    }
    echo "</tr>\n";
}
echo "</table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
