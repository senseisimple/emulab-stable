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

if (! $isadmin) {
    USERERROR("You do not have permission to view the user list!", 1);
}
if (!isset($showactive))
    $showactive = 0;
if (!isset($sortby))
    $sortby = "uid";
    
if (!$showactive) {
    echo "<b><a href='showuser_list.php3?showactive=1&sortby=$sortby'>
                Show Logged in Users</a>
          </b><br><br>\n";
}
else {
    echo "<b><a href='showuser_list.php3?showactive=0&sortby=$sortby'>
                Show All Users</a>
          </b><br><br>\n";
}

if (! strcmp($sortby, "name"))
    $order = "u.usr_name";
elseif (! strcmp($sortby, "uid"))
    $order = "u.uid";
elseif (! strcmp($sortby, "widle"))
    $order = "webidle DESC";
elseif (! strcmp($sortby, "uidle"))
    $order = "usersidle DESC";
else {
    $order = "u.uid";
}

if ($showactive) {
    $query_result =
	DBQueryFatal("SELECT u.*, ".
		     " IF(ll.time, ".
		     "    TO_DAYS(CURDATE()) - TO_DAYS(ll.time), ".
		     "    TO_DAYS(CURDATE()) - TO_DAYS(u.usr_created)) ".
		     "   as webidle, ".
		     " TO_DAYS(CURDATE()) - TO_DAYS(ull.date) as usersidle ".
		     "FROM users as u ".
		     "left join login as l on u.uid=l.uid ".
		     "left join userslastlogin as ull on u.uid=ull.uid ".
		     "left join lastlogin as ll on u.uid=ll.uid ".
		     "where l.timeout>=unix_timestamp() ".
		     "order by $order");
}
else {
    $query_result =
	DBQueryFatal("SELECT u.*, ".
		     " IF(ll.time, ".
		     "    TO_DAYS(CURDATE()) - TO_DAYS(ll.time), ".
		     "    TO_DAYS(CURDATE()) - TO_DAYS(u.usr_created)) ".
		     "   as webidle, ".
		     " TO_DAYS(CURDATE()) - TO_DAYS(ull.date) as usersidle ".
		     "FROM users as u ".
		     "left join userslastlogin as ull on u.uid=ull.uid ".
		     "left join lastlogin as ll on u.uid=ll.uid ".
		     "order by $order");
}

if (($count = mysql_num_rows($query_result)) == 0) {
    USERERROR("There are no users!", 1);
}

if ($showactive) {
    echo "<center>
          There are $count users logged in.
          </center><br>\n";
}
else {
    echo "<center>
          There are $count users.
          </center><br>\n";
}

#
# Grab the project lists and create a hash of lists, per user.
# One query instead of hundreds.
#
$projmemb_result =
    DBQueryFatal("select distinct uid,pid from group_membership order by uid");

$projmemb_array = array();

while ($row = mysql_fetch_array($projmemb_result)) {
    $uid   = $row[0];
    $pid   = $row[1];

    $projmemb_array[$uid][] = $pid;
}

echo "<table width=\"100%\" border=2 cellpadding=1 cellspacing=0
       align='center'>\n";

echo "<tr>
          <td>&nbsp</td>
          <td><a href='showuser_list.php3?showactive=$showactive&sortby=uid'>
                 UID</a></td>
          <td><a href='showuser_list.php3?showactive=$showactive&sortby=name'>
                 Name</a></td>
          <td>Projects</td>
          <td><a href='showuser_list.php3?showactive=$showactive&sortby=widle'>
                 Web<br>Idle</a></td>
          <td><a href='showuser_list.php3?showactive=$showactive&sortby=uidle'>
                 Users<br>Idle</a></td>\n";

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
    $webidle  = $row[webidle];
    $usersidle= $row[usersidle];

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

    # List of projects.
    reset($projmemb_array);
    if (isset($projmemb_array[$thisuid])) {
	echo "<td> ";
	while (list ($idx, $pid) = each($projmemb_array[$thisuid])) {
	    echo "<A href='showproject.php3?pid=$pid'>$pid</A>";
	    if ($idx != (count($projmemb_array[$thisuid]) - 1))
		echo ", ";
	}
	echo "</td>\n";
    }
    else {
	    echo "<td>--</td>\n";
    }
    
    echo "<td>$webidle</td>\n";

    if (! $usersidle)
	echo "<td>&nbsp</td>\n";
    else {
	echo "<td>$usersidle</td>\n";
    }

    if ($isadmin) {
	echo "<td align=center><A href='moduserinfo.php3?target_uid=$thisuid'>
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
