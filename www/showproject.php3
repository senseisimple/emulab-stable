<?php
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Show Project Information");


#
# Note the difference with which this page gets it arguments!
# I invoke it using GET arguments, so uid and pid are are defined
# without having to find them in URI (like most of the other pages
# find the uid).
#

#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

$isadmin = ISADMIN($uid);

#
# Verify form arguments.
# 
if (!isset($pid) ||
    strcmp($pid, "") == 0) {
    USERERROR("You must provide a project ID.", 1);
}

#
# Check to make sure thats this is a valid PID.
#
$query_result = 
    DBQueryFatal("SELECT * FROM projects WHERE pid='$pid'");
if (mysql_num_rows($query_result) == 0) {
  USERERROR("The project $pid is not a valid project.", 1);
}

#
# Verify that this uid is a member of the project being displayed.
#
if (!$isadmin) {
    $query_result = 
        DBQueryFatal("SELECT trust FROM group_membership ".
		     "WHERE uid='$uid' and pid='$pid' and gid='$pid'");
    if (mysql_num_rows($query_result) == 0) {
        USERERROR("You are not a member of Project $pid.", 1);
    }
}

SHOWPROJECT($pid, $uid);

#
# A list of project members (from the default group).
#
$query_result =
    DBQueryFatal("SELECT m.*,u.* FROM group_membership as m ".
		 "left join users as u on u.uid=m.uid ".
		 "WHERE pid='$pid' and gid='$pid'");
if (mysql_num_rows($query_result)) {
    echo "<center>
          <h3>Project Members</h3>
          </center>
          <table align=center border=1>\n";

    echo "<tr>
              <td align=center>Name</td>
              <td align=center>UID</td>
              <td align=center>Privs</td>
              <td align=center>Approved?</td>
          </tr>\n";

    while ($row = mysql_fetch_array($query_result)) {
        $target_uid = $row[uid];
	$usr_name   = $row[usr_name];
	$status     = $row[status];
	$trust      = $row[trust];

        echo "<tr>
                  <td>$usr_name</td>
                  <td>
                    <A href='showuser.php3?target_uid=$target_uid'>
                       $target_uid</A>
                  </td>
                  <td>$trust</td>\n";
	    
	if (strcmp($status, "active") == 0 ||
	    strcmp($status, "unverified") == 0) {
	    echo "<td align=center>
                      <img alt=\"Y\" src=\"greenball.gif\"></td>\n";
	}
	else {
	    echo "<td align=center>
                      <img alt=\"N\" src=\"redball.gif\"></td>\n";
	}
	echo "</tr>\n";
    }

    echo "</table>\n";
}

#
# A list of project experiments.
#
$query_result =
    DBQueryFatal("SELECT eid,expt_name FROM experiments WHERE pid='$pid'");
if (mysql_num_rows($query_result)) {
    echo "<center>
          <h3>Project Experiments</h3>
          </center>
          <table align=center border=1>\n";

    while ($row = mysql_fetch_row($query_result)) {
        $eid  = $row[0];
        $name = $row[1];
	if (!$name)
	    $name = "--";
        echo "<tr>
                  <td>
                      <A href='showexp.php3?pid=$pid&eid=$eid'>$eid</a>
                      </td>
                  <td>$name</td>
              </tr>\n";
    }

    echo "</table>\n";
}

#
# A list of project Groups (if more than just the default).
#
$query_result =
    DBQueryFatal("SELECT * FROM groups WHERE pid='$pid'");
if (mysql_num_rows($query_result)) {
    echo "<center>
          <h3>Project Groups</h3>
          </center>
          <table align=center border=1>\n";

    echo "<tr>
              <td align=center>GID</td>
              <td align=center>Desription</td>
              <td align=center>Leader</td>
          </tr>\n";

    while ($row = mysql_fetch_array($query_result)) {
        $gid      = $row[gid];
        $desc     = $row[description];
	$leader   = $row[leader];

        echo "<tr>
                  <td>
                      <A href='showgroup.php3?pid=$pid&gid=$gid'>$gid</a>
                      </td>

                  <td>$desc</td>

	          <td><A href='showuser.php3?target_uid=$leader'>$leader</A>
                      </td>
              </tr>\n";
     
    }

    echo "</table>\n";
}

echo "<p><center>
       <A href='newgroup_form.php3?pid=$pid'>Create</a> a new Group?
      </center>\n";

if ($isadmin) {
    echo "<p>
          <A href='deleteproject.php3?pid=$pid'>
             <font color=Red>Delete this project?</font></a>\n";
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
