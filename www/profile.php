<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Only known and logged in users can do this.
#
$this_user = CheckLoginOrDie(CHECKLOGIN_USERSTATUS|
			     CHECKLOGIN_WEBONLY|CHECKLOGIN_WIKIONLY);
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$optargs = OptionalPageArguments("target_user", PAGEARG_USER);

if (! isset($target_user)) {
    $target_user = $this_user;
}
$userstatus = $target_user->status();
$wikionly   = $target_user->wikionly();
$target_idx = $target_user->uid_idx();
$target_uid = $target_user->uid();


#
# Verify that this uid is a member of one of the projects that the
# target_uid is in. Must have proper permission in that group too. 
#
if (!$isadmin && 
    !$target_user->AccessCheck($this_user, $TB_USERINFO_READINFO)) {
    USERERROR("You do not have permission to view this user's information!", 1);
}

#
# Standard Testbed Header.
#
PAGEHEADER("Profile for $target_uid");

#
# See if any mailman lists owned by the user. If so we add a menu item.
#
$mm_result =
    DBQueryFatal("select owner_uid from mailman_listnames ".
		 "where owner_uid='$target_uid'");

#
# User Profile.
#
SUBPAGESTART();
SUBMENUSTART("Options");

#
# Permission check not needed; if the user can view this page, they can
# generally access these subpages, but if not, the subpage will still whine.
#
WRITESUBMENUBUTTON("My Emulab",
		   CreateURL("showuser", $target_user));

WRITESUBMENUBUTTON("Edit Profile",
		   CreateURL("moduserinfo", $target_user));

if (!$wikionly && ($isadmin || $target_user->SameUser($this_user))) {
    WRITESUBMENUBUTTON("Edit SSH Keys",
		       CreateURL("showpubkeys", $target_user));
    
    WRITESUBMENUBUTTON("Edit SFS Keys",
		       CreateURL("showsfskeys", $target_user));

    WRITESUBMENUBUTTON("Generate SSL Cert",
		       CreateURL("gensslcert", $target_user));


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
			     CreateURL("changeuid", $target_user));
    }

    if (! strcmp($userstatus, TBDB_USERSTATUS_NEWUSER) ||
	! strcmp($userstatus, TBDB_USERSTATUS_UNVERIFIED)) {
	WRITESUBMENUBUTTON("Resend Verification Key",
			     CreateURL("resendkey", $target_user));
    }
    else {
	WRITESUBMENUBUTTON("Send Test Email Message",
			     CreateURL("sendtestmsg", $target_user));
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
