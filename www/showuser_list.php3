<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Show User Information List");

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
if ($isadmin) {
    $query_result = mysql_db_query($TBDBNAME,
	"SELECT u.* FROM users as u order by u.uid");
}
else {
    $query_result = mysql_db_query($TBDBNAME,
	"select distinct u.* from users as u ".
	"left join proj_memb as p1 on u.uid=p1.uid ".
	"left join proj_memb as p2 on p1.pid=p2.pid ".
	"where p2.uid='$uid' order by u.uid");
}
if (! $query_result) {
    $err = mysql_error();
    TBERROR("Database Error getting user list: $err\n", 1);
}

if (mysql_num_rows($query_result) == 0) {
	if ($isadmin) {
	    USERERROR("There are no users!", 1);
	}
	else {
	    USERERROR("There are no users in any of your projects!", 1);
	}
}

echo "<center><h3>
      User List
      </h3></center>\n";

echo "<table width=\"100%\" border=2 cellpadding=0 cellspacing=2
       align='center'>\n";

echo "<tr>
          <td>UID</td>
          <td>Name</td>
          <td>Projects</td>
	  <td align=center>Approved?</td>\n";

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

    #
    # Suck out a list of projects too.
    #
    $projmemb_result = mysql_db_query($TBDBNAME,
	"SELECT pid FROM proj_memb where uid='$thisuid' order by pid");

    echo "<tr>
              <td><A href='showuser.php3?target_uid=$thisuid'>$thisuid</A></td>
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

    if (strcmp($status, "active") == 0 ||
	strcmp($status, "unverified") == 0) {
	echo "<td align=center><img alt=\"Y\" src=\"greenball.gif\"></td>\n";
    }
    else {
	echo "<td align=center><img alt=\"N\" src=\"redball.gif\"></td>\n";
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
