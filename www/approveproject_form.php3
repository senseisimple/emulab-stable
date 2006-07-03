<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003, 2005, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("New Project Approval");

#
# Only known and logged in users can do this.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Of course verify that this uid has admin privs!
#
$isadmin = ISADMIN($uid);
if (! $isadmin) {
    USERERROR("You do not have admin privileges to approve projects!", 1);
}

#
# Verify arguments.
# 
if (!isset($pid) ||
    strcmp($pid, "") == 0) {
    USERERROR("You must provide a project ID.", 1);
}

#
# Check to make sure thats this is a valid PID.
#
if (! TBValidProject($pid)) {
    USERERROR("The project $pid is not a valid project.", 1);
}

echo "<center><h3>You have the following choices:</h3></center>
      <table class=stealth align=center border=0>
        <tr>
            <td class=stealth>Deny</td>
            <td class=stealth>-</td>
            <td class=stealth>Deny project application (kills project records)</td>
        </tr>

        <tr>
            <td class=stealth>Destroy</td>
            <td class=stealth>-</td>
            <td class=stealth>Deny project application, and kill the user account</td>
        </tr>

        <tr>
            <td class=stealth>Approve</td>
            <td class=stealth>-</td>
            <td class=stealth>Approve the project</td>
        </tr>

        <tr>
            <td class=stealth>More Info</td>
            <td class=stealth>-</td>
            <td class=stealth>Ask for more info</td>
        </tr>

        <tr>
            <td class=stealth>Postpone</td>
            <td class=stealth>-</td>
            <td class=stealth>Twiddle your thumbs some more</td>
        </tr>
      </table>\n";

#
# Show stuff
#
SHOWPROJECT($pid, $uid);

TBProjLeader($pid, $projleader);

echo "<center>
      <h3>Project Leader Information</h3>
      </center>
      <table align=center border=0>\n";

SHOWUSER($projleader);

#
# Check to make sure that the head user is 'unapproved' or 'active'
#
$headstatus = TBUserStatus($projleader);
if (!strcmp($headstatus,TBDB_USERSTATUS_UNAPPROVED) ||
	!strcmp($headstatus,TBDB_USERSTATUS_ACTIVE)) {
    $approvable = 1;
} else {
    $approvable = 0;
}

#
# Now put up the menu choice along with a text box for an email message.
#
echo "<center>
      <h3>What would you like to do?</h3>
      </center>
      <table align=center border=1>
      <form action='approveproject.php3?pid=$pid' method='post'>\n";

echo "<tr>
          <td align=center>
              <select name=approval>
                      <option value='postpone'>Postpone</option>";
if ($approvable) {
    echo "                  <option value='approve'>Approve</option>";
}
echo "
                      <option value='moreinfo'>More Info</option>
                      <option value='deny'>Deny</option>
                      <option value='destroy'>Destroy</option>
              </select>";
if (!$approvable) {
	echo "              <br><b>WARNING:</b> Project cannot be approved,";
	echo"               since head user has not been verified";
}
echo "
          </td>
       </tr>\n";

echo "<tr>
         <td align=center>
	    <input type=checkbox value=Yep
                     name=silent>Silent (no email sent for deny,destroy)
	 </td>
       </tr>\n";

#
# Allow the approver to change the project's head UID - gotta find everyone in
# the default group, first
#
echo "<tr>
          <td align=center>
	      Head UID:
              <select name=head_uid>
                      <option value=''>(Unchanged)</option>";
$query_result =
    DBQueryFatal("select uid from group_membership where pid='$pid' and " .
	    "gid='$pid'");
while ($row = mysql_fetch_array($query_result)) {
    $thisuid = $row[uid];
    echo "                      <option value='$thisuid'>$thisuid</option>\n";
}
echo "        </select>
          </td>
       </tr>\n";

#
# Set the user interface.
#
echo "<tr>
          <td align=center>
              Default User Interface:
              <select name=user_interface>\n";

foreach ($TBDB_USER_INTERFACE_LIST as $interface) {
    echo "            <option value='$interface'>$interface</option>\n";
}
echo "        </select>
          </td>
       </tr>\n";

#
# XXX
# Temporary Plab hack.
# See if remote nodes requested and put up checkboxes to allow override.
#
$query_result =
    DBQueryFatal("select num_pcplab,num_ron from projects where pid='$pid'");

$row = mysql_fetch_array($query_result);
# These are now booleans, not actual counts.
$num_pcplab = $row[num_pcplab];
$num_ron    = $row[num_ron];

if ($num_ron || $num_pcplab) {
	echo "<tr>
                 <td align=center>\n";
	if ($num_pcplab) {
		echo "<input type=checkbox value=Yep checked
                                 name=pcplab_okay>
                                 Allow Plab &nbsp\n";
	}
	if ($num_ron) {
		echo "<input type=checkbox value=Yep checked
                                 name=ron_okay>
                                 Allow RON (PCWA) &nbsp\n";
	}
	echo "   </td>
              </tr>\n";
}

echo "<tr>
          <td>Use the text box (70 columns wide) to add a message to the
              email notification. </td>
      </tr>\n";

echo "<tr>
         <td align=center class=left>
             <textarea name=message rows=15 cols=70></textarea>
         </td>
      </tr>\n";

echo "<tr>
          <td align=center colspan=2>
              <b><input type='submit' value='Submit' name='OK'></td>
      </tr>
      </form>
      </table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
