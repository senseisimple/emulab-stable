<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2008 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Widearea Accounts Approval Form");

#
# Only admin types can use this page.
#
$this_user = CheckLoginOrDie();
$auth_usr  = $this_user->uid();
$isadmin   = ISADMIN();

if (! $isadmin) {
    USERERROR("Only testbed administrators people can access this page!", 1);
}

echo "
      <h2>Approve local accounts on specific widearea nodes</h2>

      <center>
      <h4>You have the following choices for <b>Action</b>:</h4>
      <table cellspacing=2 border=0>
        <tr>
            <td><b>Postpone</b></td>
            <td>Do nothing; application remains, pending a decision.</td>
        </tr>
        <tr>
            <td><b>Deny</b></td>
            <td>Deny user application and so notify the user.</td>
        </tr>
        <tr>
            <td><b>Nuke</b></td>
            <td>Nuke user application.  Kills user account, without
		notice to user.  Useful for bogus applications.</td>
        </tr>
        <tr>
            <td><b>Approve</b></td>
            <td>Approve the user</td>
        </tr>
      </table>
      <br />
      <h4>You have the following choices for <b>Trust</b>:</h4>
      <table cellspacing=2 cellpadding=4 border=0>
        <tr>
            <td><b>User</b></td>
            <td>User may log into the node</td>
        </tr>
        <tr>
            <td><b>Root</b></td>
            <td>User gets local root on the node</td>
        </tr>
      </table>
      <br />
      </center>
      <br />
      \n";

#
# Find all of the unapproved widearea account requests.
# 
$query_result =
    DBQueryFatal("select w.* from widearea_accounts as w ".
		 "left join users as u on u.uid_idx=w.uid_idx ".
		 "WHERE u.status!='" . TBDB_USERSTATUS_UNVERIFIED . "' and ".
		 "u.status!='" . TBDB_USERSTATUS_NEWUSER . "' and ".
		 "w.trust='" . TBDB_TRUSTSTRING_NONE . "'");

if (mysql_num_rows($query_result) == 0) {
    USERERROR("There are no new accounts that need approval.", 1);
}

#
# Now build a table with a bunch of selections. The thing to note about the
# form inside this table is that the selection fields are constructed with
# name= on the fly, from the uid of the user to be approved. In other words:
#
#             uid     menu     node_id
#	name=stoller$$approval-wa33 value=approved,denied,postpone
#	name=stoller$$trust-wa33 value=user,local_root
#
# so that we can go through the entire list of post variables, looking
# for these. The alternative is to work backwards, and I do not like that.
# 
echo "<table width=\"100%\" border=2 cellpadding=2 cellspacing=2
       align=\"center\">\n";

echo "<tr>
          <th rowspan=2>User</th>
          <th rowspan=2>Node ID</th>
          <th rowspan=2>Date<br>Applied</th>
          <th rowspan=2>Action</th>
          <th rowspan=2>Trust</th>
          <th>Name</th>
          <th>Title</th>
          <th>Affil</th>
          <th>E-mail</th>
          <th>Phone</th>
      </tr>
      <tr>
          <th colspan=5>Address</th>
      </tr>\n";

echo "<form action='approvewauser.php3' method='post'>\n";

while ($usersrow = mysql_fetch_array($query_result)) {
    $newuid        = $usersrow["uid"];
    $node_id       = $usersrow["node_id"];
    $date_applied  = $usersrow["date_applied"];

    #
    # Cause this field was added late and might be null.
    # 
    if (! $date_applied) {
	$date_applied = "--";
    }

    if (! ($user = User::Lookup($newuid))) {
	TBERROR("Could not lookup user $uid_idx", 1);
    }

    $name	= $user->name();
    $email	= $user->email();
    $title	= $user->title();
    $affil	= $user->affil();
    $addr	= $user->addr();
    $addr2	= $user->addr2();
    $city	= $user->city();
    $state	= $user->state();
    $zip	= $user->zip();
    $country	= $user->country();
    $phone	= $user->phone();

    echo "<tr>
              <td colspan=10> </td>
          </tr>
          <tr>
              <td rowspan=2>$newuid</td>
              <td rowspan=2>
                  <A href='shownode.php3?node_id=$node_id'>$node_id</a></td>
              <td rowspan=2>$date_applied</td>
              <td rowspan=2>
                  <select name=\"$newuid\$\$approval-$node_id\">
                          <option value='postpone'>Postpone </option>
                          <option value='approve'>Approve </option>
                          <option value='deny'>Deny </option>
                          <option value='nuke'>Nuke </option>
                  </select>
              </td>
              <td rowspan=2>
                  <select name=\"$newuid\$\$trust-$node_id\">
                          <option value='user'>User </option>
                          <option value='local_root'>Root </option>\n";
    echo "        </select>
              </td>\n";

    echo "    <td>&nbsp;$name&nbsp;</td>
              <td>&nbsp;$title&nbsp;</td>
              <td>&nbsp;$affil&nbsp;</td>
              <td>&nbsp;$email&nbsp;</td>
              <td>&nbsp;$phone&nbsp;</td>
          </tr>\n";
    echo "<tr>
              <td colspan=5>&nbsp;$addr&nbsp;";
    if (strcmp($addr2,"")) { 
	echo "&nbsp;$addr2&nbsp;"; 
    }
    echo "                  &nbsp;$city&nbsp;
                            &nbsp;$state&nbsp;
                            &nbsp;$zip&nbsp;
                            &nbsp;$country&nbsp;</td>
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
