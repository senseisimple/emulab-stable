<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("New Users Approval Form");

#
# Only known and logged in users can be verified.
#
$auth_usr = GETLOGIN();
LOGGEDINORDIE($auth_usr);

echo "
      <h2>Approve new users in your Project or Group</h2>
      Use this page to approve new members of your Project or Group.  Once
      approved, they will be able to log into machines in your Project's 
      experiments. Be sure to toggle the menu options appropriately for
      each pending user.

      <p>
      <table cellspacing=2 border=0>
        <tr>
            <td colspan=4>
                <h4>You have the following choices for <b>Action</b>:</td>
        <tr>
        <tr>
            <td>&nbsp</td>
            <td>Postpone</td>
            <td>-</td>
            <td>Do nothing; application remains, pending a decision.</td>
        </tr>
        <tr>
            <td>&nbsp</td>
            <td>Deny</td>
            <td>-</td>
            <td>Deny user application and so notify the user.</td>
        </tr>
        <tr>
            <td>&nbsp</td>
            <td>Nuke</td>
            <td>-</td>
            <td>Nuke user application.  Kills user account, without
		notice to user.  Useful for
                bogus project applications.</td>
        </tr>
        <tr>
            <td>&nbsp</td>
            <td>Approve</td>
            <td>-</td>
            <td>Approve the user</td>
        </tr>
      </table>
      </center>
      <p>
      <table cellspacing=2 border=0>
        <tr>
            <td colspan=4>
                <h4>You have the following choices for <b>Trust</b>:</td>
        <tr>
        <tr>
            <td>&nbsp</td>
            <td>User</td>
            <td>-</td>
            <td>User may log into machines in your experiments</td>
        </tr>
        <tr>
            <td>&nbsp</td>
            <td>Local Root</td>
            <td>-</td>
            <td>User may create/destroy experiments in your project and
                has root privileges on machines in your experiments</td>
        </tr>
        <tr>
            <td>&nbsp</td>
            <td>Group Root</td>
            <td>-</td>
            <td>In addition to Local Root privileges, user may also
                approve new group members and 
                modify user info for other users within the group. This
                level of trust is typically given only to TAs and the
                like.</td>
        </tr>
      </table>

      <center>
      <b>Important group
       <a href='docwrapper.php3?docname=groups.html#SECURITY'>
       security issues</a> are discussed in the
       <a href='docwrapper.php3?docname=groups.html'>Groups Tutorial</a>
      </b>
      </center><br>

      \n";

#
# Find all of the groups that this person has project/group root in, and 
# then in all of those groups, all of the people who are awaiting to be
# approved (status = none).
#
# First off, just determine if this person has group/project root anywhere.
#
$query_result =
    DBQueryFatal("SELECT pid FROM group_membership WHERE uid='$auth_usr' ".
		 "and (trust='group_root' or trust='project_root')");
if (mysql_num_rows($query_result) == 0) {
    USERERROR("You do not have Root permissions in any Project or Group.", 1);
}

#
# Okay, so this operation sucks out the right people by joining the
# group_membership table with itself. Kinda obtuse if you are not a natural
# DB guy. Sorry. Well, obtuse to me.
# 
$query_result =
    DBQueryFatal("SELECT g.* FROM group_membership as g ".
		 "LEFT JOIN group_membership as authed ".
		 "ON g.pid=authed.pid and g.gid=authed.gid and ".
		 "   g.uid!='$auth_usr' and g.trust='none' ".
		 "left join users as u on u.uid=g.uid ".
		 "WHERE u.status='" . TBDB_USERSTATUS_UNAPPROVED . "' and ".
		 "      authed.uid='$auth_usr' and ".
		 "      (authed.trust='group_root' or ".
		 "       authed.trust='project_root') ".
		 "ORDER BY g.uid,g.pid,g.gid");

if (mysql_num_rows($query_result) == 0) {
    USERERROR("You have no new project members who need approval.", 1);
}

#
# Now build a table with a bunch of selections. The thing to note about the
# form inside this table is that the selection fields are constructed with
# name= on the fly, from the uid of the user to be approved. In other words:
#
#             uid     menu     project/group
#	name=stoller$$approval-testbed/testbed value=approved,denied,postpone
#	name=stoller$$trust-testbed/testbed value=user,local_root
#
# so that we can go through the entire list of post variables, looking
# for these. The alternative is to work backwards, and I do not like that.
# 
echo "<table width=\"100%\" border=2 cellpadding=2 cellspacing=2
       align=\"center\">\n";

echo "<tr>
          <td rowspan=2>User</td>
          <td rowspan=2>Project</td>
          <td rowspan=2>Group</td>
          <td rowspan=2>Date<br>Applied</td>
          <td rowspan=2>Action</td>
          <td rowspan=2>Trust</td>
          <td>Name</td>
          <td>Title</td>
          <td>Affil</td>
          <td>E-mail</td>
          <td>Phone</td>
      </tr>
      <tr>
          <td>Addr</td>
          <td>Addr2</td>
          <td>City</td>
          <td>State</td>
          <td>Zip</td>
      </tr>\n";

echo "<form action='approveuser.php3' method='post'>\n";

while ($usersrow = mysql_fetch_array($query_result)) {
    $newuid        = $usersrow[uid];
    $pid           = $usersrow[pid];
    $gid           = $usersrow[gid];
    $date_applied  = $usersrow[date_applied];

    #
    # Cause this field was added late and might be null.
    # 
    if (! $date_applied) {
	$date_applied = "--";
    }

    #
    # Only project leaders get to add someone as group root.
    # 
    TBProjLeader($pid, $projleader);
    if (strcmp($auth_usr, $projleader) == 0) {
	    $isleader = 1;
    }
    else {
	    $isleader = 0;
    }

    $userinfo_result =
	DBQueryFatal("SELECT * from users where uid='$newuid'");

    $row	= mysql_fetch_array($userinfo_result);
    $name	= $row[usr_name];
    $email	= $row[usr_email];
    $title	= $row[usr_title];
    $affil	= $row[usr_affil];
    $addr	= $row[usr_addr];
    $addr2	= $row[usr_addr2];
    $city	= $row[usr_city];
    $state	= $row[usr_state];
    $zip	= $row[usr_zip];
    $phone	= $row[usr_phone];

    echo "<tr>
              <td colspan=10> </td>
          </tr>
          <tr>
              <td rowspan=2>$newuid</td>
              <td rowspan=2>$pid</td>
              <td rowspan=2>$gid</td>
              <td rowspan=2>$date_applied</td>
              <td rowspan=2>
                  <select name=\"$newuid\$\$approval-$pid/$gid\">
                          <option value='postpone'>Postpone </option>
                          <option value='approve'>Approve </option>
                          <option value='deny'>Deny </option>
                          <option value='nuke'>Nuke </option>
                  </select>
              </td>
              <td rowspan=2>
                  <select name=\"$newuid\$\$trust-$pid/$gid\">
                          <option value='user'>User </option>
                          <option value='local_root'>Local Root </option>\n";
    if ($isleader) {
	    echo "        <option value='group_root'>Group Root </option>\n";
    }
    echo "        </select>
              </td>\n";

    echo "    <td>&nbsp;$name&nbsp;</td>
              <td>&nbsp;$title&nbsp;</td>
              <td>&nbsp;$affil&nbsp;</td>
              <td>&nbsp;$email&nbsp;</td>
              <td>&nbsp;$phone&nbsp;</td>
          </tr>\n";
    echo "<tr>
              <td>&nbsp;$addr&nbsp;</td>
              <td>&nbsp;$addr2&nbsp;</td>
              <td>&nbsp;$city&nbsp;</td>
              <td>&nbsp;$state&nbsp;</td>
              <td>&nbsp;$zip&nbsp;</td>
          </tr>\n";
}
echo "<tr>
          <td align=center colspan=11>
              <b><input type='submit' value='Submit' name='OK'></td>
      </tr>
      </form>
      </table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
