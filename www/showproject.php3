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
$query_result = mysql_db_query($TBDBNAME,
	"SELECT * FROM projects WHERE pid=\"$pid\"");
if (mysql_num_rows($query_result) == 0) {
  USERERROR("The project $pid is not a valid project.", 1);
}

#
# Verify that this uid is a member of the project for the experiment
# being displayed, or is an admin person.
#
if (!$isadmin) {
    $query_result = mysql_db_query($TBDBNAME,
	"SELECT pid FROM proj_memb WHERE uid=\"$uid\" and pid=\"$pid\"");
    if (mysql_num_rows($query_result) == 0) {
        USERERROR("You are not a member of Project $pid.", 1);
    }
}

echo "<center>
      <h3>Project Information</h3>
      </center>\n";
SHOWPROJECT($pid, $uid);

#
# A list of project members.
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT p.*,u.* FROM proj_memb as p ".
	"left join users as u on u.uid=p.uid ".
        "WHERE pid=\"$pid\"");
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

	if (strcmp($trust, "local_root") == 0 ||
	    strcmp($trust, "group_root") == 0) {
	    $trust = "root";
	}
	
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
$query_result = mysql_db_query($TBDBNAME,
	"SELECT eid,expt_name FROM experiments WHERE pid=\"$pid\"");
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
                      <A href='showexp.php3?exp_pideid=$pid\$\$$eid'>$eid</a>
                      </td>
                  <td>$name</td>
              </tr>\n";
    }

    echo "</table>\n";
}

echo "</center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
