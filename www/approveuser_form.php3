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
      <h2>Approve new users in your Project</h2>
      Use this page to approve new members of your Project.  Once
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
            <td>Do nothing; user remains pending decision</td>
        </tr>
        <tr>
            <td>&nbsp</td>
            <td>Deny</td>
            <td>-</td>
            <td>Deny user application</td>
        </tr>
        <tr>
            <td>&nbsp</td>
            <td>Nuke</td>
            <td>-</td>
            <td>Nuke user application. Kills user account. Useful for
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
            <td>Root</td>
            <td>-</td>
            <td>User may create/destroy experiments in your project and
                has root privledges on machines in your experiments</td>
        </tr>
      </table>
      \n";

#
# Find all of the groups that this person has group_root in, and then in
# all of those groups, all of the people who are awaiting to be approved
# (status = none).
#
# First off, just determine if this person has group_root anywhere.
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT pid FROM proj_memb WHERE uid='$auth_usr' ".
                "and trust='group_root'");
if (! $query_result) {
    $err = mysql_error();
    TBERROR("Database Error getting project info for $auth_usr: $err\n", 1);
}
if (mysql_num_rows($query_result) == 0) {
    USERERROR("You do not have Project Root permissions in any Project.", 1);
}

#
# Okay, so this operation sucks out the right people by joining the
# proj_memb table with itself. Kinda obtuse if you are not a natural
# DB guy. Sorry. Well, obtuse to me.
# 
$query_result = mysql_db_query($TBDBNAME,
	"SELECT proj_memb.* ".
        "FROM proj_memb LEFT JOIN proj_memb as authed ".
        "ON proj_memb.pid=authed.pid and proj_memb.uid!='$auth_usr' ".
           "and proj_memb.trust='none' ".
        "WHERE authed.uid='$auth_usr' and authed.trust='group_root'");
if (! $query_result) {
    $err = mysql_error();
    TBERROR("Database Error getting approvable users for $auth_usr: $err\n",
             1);
}
if (mysql_num_rows($query_result) == 0) {
    USERERROR("You have no new project members who need approval.", 1);
}

#
# Now build a table with a bunch of selections. The thing to note about the
# form inside this table is that the selection fields are constructed with
# name= on the fly, from the uid of the user to be approved. In other words:
#
#             uid     menu     project
#	name=stoller$$approval-testbed value=approved,denied,postpone
#	name=stoller$$trust-testbed value=user,local_root
#
# so that we can go through the entire list of post variables, looking
# for these. The alternative is to work backwards, and I do not like that.
# 
echo "<table width=\"100%\" border=2 cellpadding=2 cellspacing=2
       align=\"center\">\n";

echo "<tr>
          <td rowspan=2>User</td>
          <td rowspan=2>Project</td>
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
    $date_applied  = $usersrow[date_applied];

    #
    # Cause this field was added late and might be null.
    # 
    if (! $date_applied) {
	$date_applied = "--";
    }

    $userinfo_result = mysql_db_query($TBDBNAME,
	"SELECT * from users where uid=\"$newuid\"");

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
              <td rowspan=2>$date_applied</td>
              <td rowspan=2>
                  <select name=\"$newuid\$\$approval-$pid\">
                          <option value='postpone'>Postpone</option>
                          <option value='approve'>Approve</option>
                          <option value='deny'>Deny</option>
                          <option value='nuke'>Nuke</option>
                  </select>
              </td>
              <td rowspan=2>
                  <select name=\"$newuid\$\$trust-$pid\">
                          <option value='user'>User</option>
                          <option value='local_root'>Root</option>
                  </select>
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
          <td align=center colspan=10>
              <b><input type='submit' value='Submit' name='OK'></td>
      </tr>
      </form>
      </table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
