<?php
include("defs.php3");
include("showstuff.php3");

#
# Only known and logged in users can do this.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

$isadmin = ISADMIN($uid);

#
# Verify form arguments.
# 
if (!isset($target_uid) ||
    strcmp($target_uid, "") == 0) {
    USERERROR("You must provide a User ID.", 1);
}

#
# Standard Testbed Header, now that we know what we want to say.
#
if (strcmp($uid, $target_uid)) {
    PAGEHEADER("Information for User: $target_uid");
}
else {
    PAGEHEADER("My Emulab.Net");
}

#
# Check to make sure thats this is a valid UID.
#
$query_result =
    DBQueryFatal("SELECT * FROM users WHERE uid='$target_uid'");
if (mysql_num_rows($query_result) == 0) {
  USERERROR("The user $target_uid is not a valid user", 1);
}

#
# Verify that this uid is a member of one of the projects that the
# target_uid is in. Must have proper permission in that group too. 
#
if (!$isadmin &&
    strcmp($uid, $target_uid)) {

    if (! TBUserInfoAccessCheck($uid, $target_uid, $TB_USERINFO_READINFO)) {
	USERERROR("You do not have permission to view this user's ".
		  "information!", 1);
    }
}

#
# Tell the user how many PCs he is using.
#
$yourpcs = TBUserPCs($target_uid);

if ($yourpcs) {
    echo "<center><font color=Red size=+1>\n";
    
    if (strcmp($uid, $target_uid))
	echo "$target_uid is using $yourpcs PCs!\n";
    else
	echo "You are using $yourpcs PCs!\n";
    
    echo "</font></center>\n";
}

#
# Lets show Experiments.
#
$query_result =
    DBQueryFatal("select * from experiments  ".
		 "where expt_head_uid='$target_uid' order by pid,eid");

if (mysql_num_rows($query_result)) {
    echo "<center>
          <h3>Current Experiments</h3>
          </center>
          <table align=center border=1 cellpadding=1 cellspacing=2>\n";

    echo "<tr>
              <td align=center>PID</td>
              <td align=center>EID</td>
              <td align=center>Name</td>
          </tr>\n";

    while ($projrow = mysql_fetch_array($query_result)) {
	$pid  = $projrow[pid];
	$eid  = $projrow[eid];
	$name = $projrow[expt_name];

        echo "<tr>
                 <td><A href='showproject.php3?pid=$pid'>$pid</A></td>
                 <td><A href='showexp.php3?pid=$pid&eid=$eid'>$eid</A></td>
                 <td>$name</td>
             </tr>\n";
    }
    echo "</table>\n";
}

echo "</center>\n";

#
# Lets show projects.
#
$query_result =
    DBQueryFatal("select distinct g.pid,p.name from group_membership as g ".
		 "left join projects as p on p.pid=g.pid ".
		 "where uid='$target_uid' order by pid");

if (mysql_num_rows($query_result)) {
    echo "<center>
          <h3>Project Membership</h3>
          </center>
          <table align=center border=1 cellpadding=1 cellspacing=2>\n";

    echo "<tr>
              <td align=center>PID</td>
              <td align=center>Name</td>
          </tr>\n";

    while ($projrow = mysql_fetch_array($query_result)) {
	$pid  = $projrow[pid];
	$name = $projrow[name];

        echo "<tr>
                 <td><A href='showproject.php3?pid=$pid'>$pid</A></td>
                 <td>$name</td>
             </tr>\n";
    }
    echo "</table>\n";
}

#
# Sub group membership too.
# 
SHOWGROUPMEMBERSHIP($target_uid);

#
# User Profile.
#
echo "<center>
      <h3>Profile</h3>
      </center>\n";

SHOWUSER($target_uid);

#
# Edit option.
#
if ($isadmin ||
    TBUserInfoAccessCheck($uid, $target_uid, $TB_USERINFO_MODIFYINFO)) {

    echo "<br><br><center>
           <A href='modusr_form.php3?target_uid=$target_uid'>
              Edit Profile?</a>
         </center>\n";
}
    
#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
