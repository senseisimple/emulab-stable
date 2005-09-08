<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Mailman Lists");

#
#
# Only known and logged in users allowed.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

if (! isset($sortby)) {
    $sortby = "listname";
}

if ($sortby == "listname") {
    $order = "mm.listname";
}
elseif ($sortby == "uid") {
    $order = "mm.owner_uid";
}
else {
    $order = "mm.listname";
}

#
# Allow admin users to view the lists for a specific uid.
#
if (isset($target_uid) && $target_uid != "") {
    if ($target_uid != $uid && !$isadmin) {
	USERERROR("You do not have permission to list mailman lists for ".
		  "other users!", 1);
    }
}
else {
    $target_uid = $uid;
}

# Sanity check the uid.
if (! TBvalid_uid($target_uid)) {
    PAGEARGERROR("Invalid characters in $target_uid");
}


SUBPAGESTART();
SUBMENUSTART("More Options");
WRITESUBMENUBUTTON("Create a Mailman List", "newmmlist.php3");
SUBMENUEND();

#
# Ask the mailman server for the users lists. 
#
SUEXEC($uid, "nobody", "webmmlistmembership $uid", SUEXEC_ACTION_DIE);

#
# Parse the suexec output into a table.
#
if (count($suexec_output_array)) {
    echo "<br>
           <center><font size=+1>
          You are a member of the following Mailman lists
          </font></center><br>\n";
    
    echo "<table border=2 cellpadding=0 cellspacing=2
           align='center'>\n";

    echo "<tr>
              <th>List Name</th>
              <th>Archives</th>
              <th>Configure</th>
          </tr>\n";
    

    for ($i = 0; $i < count($suexec_output_array); $i++) {
	$listname = $suexec_output_array[$i];

	echo "<tr>
                  <td><a href='mailto:$listname@${OURDOMAIN}'>$listname</a>
                       </td>
                  <td align=center>
                       <a href='gotommlist.php3?listname=$listname'>
                         <img src=\"arrow4.ico\"
                         alt='archive' border=0></A></td>
                  <td align=center>
                       <a href='gotommlist.php3?listname=$listname".
	                  "&wantconfig=1'>
                         <img src=\"arrow4.ico\"
                              alt='config' border=0></A></td>
              </tr>\n";
    }
    echo "</table><br>\n";
}

#
# See if the target is the owner of any lists.
#
if ($isadmin && !isset($target_uid)) {
    $query_result =
	DBQueryFatal("select mm.* from mailman_listnames as mm ".
		     "order by $order");
}
else {
    #
    # View lists for a specific user (might be current user)
    # 
    $query_result =
	DBQueryFatal("select mm.* from mailman_listnames as mm ".
		     "where mm.owner_uid='$target_uid' ".
		     "order by $order");
}

if (mysql_num_rows($query_result)) {
    echo "<br>
           <center><font size=+1>
          You are the administrator for the following Mailman lists
          </font></center><br>\n";
    
    echo "<table border=2 cellpadding=0 cellspacing=2
           align='center'>\n";

    echo "<tr>
              <th><a href='showmmlists.php3?&sortby=listname".
	           "&target_uid=$target_uid'>
                  List Name</th>
              <th><a href='showmmlists.php3?&sortby=uid'".
	           "&target_uid=$target_uid'>
                  Owner</th>
              <th>Admin Page</th>
              <th>Reset Password</th>
              <th>Delete List</th>
          </tr>\n";

    while ($row = mysql_fetch_array($query_result)) {
	$listname  = $row['listname'];
	$owner_uid = $row['owner_uid'];
	$mmurl     = "gotommlist?listname=${listname}";
	
	echo "<tr>
                  <td><a href='mailto:$listname@${OURDOMAIN}'>$listname</a>
                       </td>
                  <td><A href='showuser.php3?target_uid=$owner_uid'>
                            $owner_uid</A></td>
                  <td align=center><A href='${mmurl}&wantadmin=1'>
                         <img src=\"arrow4.ico\"
                              border=0 alt='admin'></A></td>
                  <td align=center>
                       <A href='${mmurl}&wantadmin=1&link=passwords'>
                           <img src='/autostatus-icons/yellowball.gif'
                                border=0 alt=passwd>
                        </A></td>
                  <td align=center>
                       <A href='delmmlist.php3?listname=$listname'>
                           <img src='/autostatus-icons/redball.gif'
                                border=0 alt=delete>
                        </A></td>\n";
        echo "</tr>\n";
    }
    echo "</table>\n";
}
SUBPAGEEND();

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
