<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");
include_once("template_defs.php");

#
# Only known and logged in users can do this.
#
$this_user = CheckLoginOrDie(CHECKLOGIN_USERSTATUS|
			     CHECKLOGIN_WEBONLY|CHECKLOGIN_WIKIONLY);
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify form arguments.
# 
if (!isset($user) ||
    strcmp($user, "") == 0) {
    USERERROR("You must provide a User ID.", 1);
}

#
# Check to make sure thats this is a valid UID.
#
if (! ($target_user = User::Lookup($user))) {
    USERERROR("Could not lookup user '$user'!", 1);
}
$userstatus = $target_user->status();
$wikionly   = $target_user->wikionly();
$target_idx = $target_user->uid_idx();
$target_uid = $target_user->uid();

#
# Standard Testbed Header, now that we know what we want to say.
#
if (! $this_user->SameUser($target_user)) {
    PAGEHEADER("Information for User: $target_uid");
}
else {
    PAGEHEADER("My Emulab.Net");
}

#
# Verify that this uid is a member of one of the projects that the
# target_uid is in. Must have proper permission in that group too. 
#
if (!$isadmin && 
    !$target_user->AccessCheck($this_user, $TB_USERINFO_READINFO)) {
    USERERROR("You do not have permission to view this user's information!", 1);
}

#
# Special banner message.
#
$message = TBGetSiteVar("web/banner");
if ($message != "") {
    echo "<center><font color=Red size=+1>\n";
    echo "$message\n";
    echo "</font></center><br>\n";
}

#
# Tell the user how many PCs he is using.
#
$yourpcs = $target_user->PCsInUse();

if ($yourpcs) {
    echo "<center><font color=Red size=+2>\n";
    
    if (! $this_user->SameUser($target_user))
	echo "$target_uid is using $yourpcs PCs!\n";
    else
	echo "You are using $yourpcs PCs!\n";
    
    echo "</font></center>\n";
}

#
# Put a link down to the profile
#
echo "<h3><a href=\"#PROFILE\">Manage User Profile</a></h3>\n";

if ($EXPOSETEMPLATES) {
    SHOWTEMPLATELIST("USER", 0, $uid, $target_uid);
}

#
# Lets show Experiments.
#
SHOWEXPLIST("USER", $uid, $target_uid);

#
# Lets show project and group membership.
#
$query_result =
    DBQueryFatal("select distinct g.pid,g.gid,g.trust,p.name,gr.description, ".
    		 "       count(distinct r.node_id) as ncount ".
		 " from group_membership as g ".
		 "left join projects as p on p.pid=g.pid ".
		 "left join groups as gr on gr.pid=g.pid and gr.gid=g.gid ".
		 "left join experiments as e on g.pid=e.pid and g.gid=e.gid ".
		 "left join reserved as r on e.pid=r.pid and e.eid=r.eid ".
		 "left join group_membership as g2 on g2.pid=g.pid and ".
		 "     g2.gid=g.gid and ".
		 "     g2.uid_idx='" . $this_user->uid_idx() . "' ".
		 "where g.uid_idx='$target_idx' and ".
		 ($isadmin ? "" : "g2.uid_idx is not null and ") .
		 "g.trust!='" . TBDB_TRUSTSTRING_NONE . "' ".
		 "group by g.pid, g.gid ".
		 "order by g.pid,gr.created");

if (mysql_num_rows($query_result)) {
    echo "<center>
          <h3>Project and Group Membership</h3>
          </center>
          <table align=center border=1 cellpadding=1 cellspacing=2>\n";

    echo "<tr>
              <th>PID</th>
              <th>GID</th>
	      <th>Nodes</th>
              <th>Name/Description</th>
              <th>Trust</th>
              <th>MailTo</th>
          </tr>\n";

    while ($projrow = mysql_fetch_array($query_result)) {
	$pid   = $projrow[pid];
	$gid   = $projrow[gid];
	$name  = $projrow[name];
	$desc  = $projrow[description];
	$trust = $projrow[trust];
	$nodes = $projrow[ncount];
	
	if (TBTrustConvert($trust) != $TBDB_TRUST_NONE) {
	    echo "<tr>
                     <td><A href='showproject.php3?pid=$pid'>
                            $pid</A></td>
                     <td><A href='showgroup.php3?pid=$pid&gid=$gid'>
                            $gid</A></td>\n";

	    echo "<td>$nodes</td>\n";

	    if (strcmp($pid,$gid)) {
		echo "<td>$desc</td>\n";
		$mail  = $pid . "-" . $gid . "-users@" . $OURDOMAIN;
	    }
	    else {
		echo "<td>$name</td>\n";
		$mail  = $pid . "-users@" . $OURDOMAIN;
	    }
	    echo "<td>$trust</td>\n";

	    if ($MAILMANSUPPORT) {
		# Not sure what I want to do here ...
		echo "<td nowrap><a href=mailto:$mail>$mail</a></td>";
	    }
	    else {
		echo "<td nowrap><a href=mailto:$mail>$mail</a></td>";
	    }
	    echo "</tr>\n";
        }
    }
    echo "</table>\n";

    echo "<center>
          Click on the GID to view/edit group membership and trust levels.
          </center>\n";
}

#
# Widearea Accounts
# 
SHOWWIDEAREAACCOUNTS($target_uid);

#
# User Profile.
#
echo "<a NAME=PROFILE></a>\n";
echo "<center>
      <h3>User Profile</h3>
      </center>\n";

#
# See if any mailman lists owned by the user. If so we add a menu item.
#
$mm_result =
    DBQueryFatal("select owner_uid from mailman_listnames ".
		 "where owner_uid='$target_uid'");

SUBPAGESTART();
SUBMENUSTART("User Options");
#
# Permission check not needed; if the user can view this page, they can
# generally access these subpages, but if not, the subpage will still whine.
# 
WRITESUBMENUBUTTON("Edit Profile",
		   CreateURL("moduserinfo", $target_user));

if (!$wikionly && ($isadmin || $target_user->SameUser($this_user))) {
    WRITESUBMENUBUTTON("Edit SSH Keys",
		       CreateURL("showpubkeys", $target_user));
    
    WRITESUBMENUBUTTON("Edit SFS Keys",
		       CreateURL("showsfskeys", $target_user));

    WRITESUBMENUBUTTON("Generate SSL Cert",
		       CreateURL("gensslcert", $target_user));

    WRITESUBMENUBUTTON("Show History",
		       CreateURL("showstats", $target_user, "showby", "user"));

    if ($MAILMANSUPPORT && mysql_num_rows($mm_result)) {
	WRITESUBMENUBUTTON("Show Mailman Lists",
			   CreateURL("showmmlists", $target_user));
    }
}

if ($isadmin) {
    SUBMENUSECTION("Admin Options");
    
    if (!strcmp($userstatus, TBDB_USERSTATUS_FROZEN)) {
	WRITESUBMENUBUTTON("Thaw User",
			   CreateURL("freezeuser", $target_user,
				     "action", "thaw"));
    }
    else {
	WRITESUBMENUBUTTON("Freeze User",
			   CreateURL("freezeuser", $target_user,
				     "action", "freeze"));
    }
    WRITESUBMENUBUTTON("Delete User",
		       CreateURL("deleteuser", $target_user));

    WRITESUBMENUBUTTON("SU as User",
		       CreateURL("suuser", $target_user));

    if ($userstatus == TBDB_USERSTATUS_UNAPPROVED) {
	WRITESUBMENUBUTTON("Change UID",
			   CreateURL("changeuid", $this_user));
    }

    if (! strcmp($userstatus, TBDB_USERSTATUS_NEWUSER) ||
	! strcmp($userstatus, TBDB_USERSTATUS_UNVERIFIED)) {
	WRITESUBMENUBUTTON("Resend Verification Key",
			   CreateURL("resendkey", $this_user));
    }
    else {
	WRITESUBMENUBUTTON("Send Test Email Message",
			   CreateURL("sendtestmsg", $this_user));
    }
}
SUBMENUEND();

$target_user->Show();

SUBPAGEEND();

if ($isadmin) {
    echo "<center>
          <h3>User Stats</h3>
         </center>\n";

    echo $target_user->ShowStats();
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>



