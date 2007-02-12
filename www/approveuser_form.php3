<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003, 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("New User Approval");

#
# Only known and logged in users can be verified.
#
$this_user   = CheckLoginOrDie();
$auth_usr    = $this_user->uid();
$auth_usridx = $this_user->uid_idx();

#
# The reason for this call is to make sure that globals are set properly.
#
$reqargs = RequiredPageArguments();

#
# Find all of the groups that this person has project/group root in, and 
# then in all of those groups, all of the people who are awaiting to be
# approved (status = none).
#
$approvelist = $this_user->ApprovalList(1);

if (count($approvelist) == 0) {
    USERERROR("You have no new project members who need approval.", 1);
}

echo "
      <h2>Approve new users in your Project or Group</h2>
      <p>
      Use this page to approve new members of your Project or Group.  Once
      approved, they will be able to log into machines in your Project's 
      experiments. Be sure to toggle the menu options appropriately for
      each pending user.
      </p>

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
		notice to user.  Useful for
                bogus project applications.</td>
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
            <td>User may log into machines in your experiments</td>
        </tr>
        <tr>
            <td><b>Local Root</b></td>
            <td>User may create/destroy experiments in your project and
                has root privileges on machines in your experiments</td>
        </tr>
        <tr>
            <td><b>Group Root</b></td>
            <td>In addition to Local Root privileges, user may also
                approve new group members and 
                modify user info for other users within the group. This
                level of trust is typically given only to TAs and the
                like.</td>
        </tr>
      </table>
      <br />
      <b>Important group
       <a href='docwrapper.php3?docname=groups.html#SECURITY'>
       security issues</a> are discussed in the
       <a href='docwrapper.php3?docname=groups.html'>Groups Tutorial</a>.
      </b>
      </center><br />

      \n";

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
          <th rowspan=2>User</th>
          <th rowspan=2>Project</th>
          <th rowspan=2>Group</th>
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

echo "<form action='approveuser.php3' method='post'>\n";

while (list ($uid_idx, $grouplist) = each ($approvelist)) {
  if (! ($user = User::Lookup($uid_idx))) {
    TBERROR("Could not lookup user $uid_idx", 1);
  }

  # Iterate over groups for this user.
  for ($i = 0; $i < count($grouplist); $i++) {
    $group        = $grouplist[$i];
    
    $newuid       = $user->uid();
    $gid          = $group->gid();
    $gid_idx      = $group->gid_idx();
    $pid          = $group->pid();
    $pid_idx      = $group->pid_idx();

    $group->MemberShipInfo($user, $trust, $date_applied, $date_approved);

    #
    # Cause this field was added late and might be null.
    # 
    if (! $date_applied) {
	$date_applied = "--";
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
              <td rowspan=2>$newuid</td>
              <td rowspan=2>$pid</td>
              <td rowspan=2>$gid</td>
              <td rowspan=2>$date_applied</td>
              <td rowspan=2>
                  <select name=\"U${uid_idx}\$\$approval-$pid/$gid\">
                          <option value='postpone'>Postpone </option>
                          <option value='approve'>Approve </option>
                          <option value='deny'>Deny </option>
                          <option value='nuke'>Nuke </option>
                  </select>
              </td>
              <td rowspan=2>
                  <select name=\"U${uid_idx}\$\$trust-$pid/$gid\">\n";
     
    if ($group->CheckTrustConsistency($user, TBDB_TRUSTSTRING_USER, 0)) {
	echo  "<option value='user'>User </option>\n";
    }
    if ($group->CheckTrustConsistency($user, TBDB_TRUSTSTRING_LOCALROOT, 0)) {
	# local_root means any root is valid.
        echo  "<option value='local_root'>Local Root </option>\n";

	# Allowed to set to group root?
	if ($group->AccessCheck($this_user, $TB_PROJECT_BESTOWGROUPROOT)) {
	    echo  "<option value='group_root'>Group Root </option>\n";
	}
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
