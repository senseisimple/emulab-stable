<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003, 2005, 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Only known and logged in users can be verified. 
#
$this_user = CheckLoginOrDie(CHECKLOGIN_UNVERIFIED|CHECKLOGIN_NEWUSER|
			     CHECKLOGIN_WEBONLY|CHECKLOGIN_WIKIONLY);
$uid       = $this_user->uid();

#
# Must provide the key!
#
$optargs = OptionalPageArguments("key", PAGEARG_STRING);

if (!isset($key) || strcmp($key, "") == 0) {
    USERERROR("Missing field; ".
              "Please go back and fill out the \"key\" field!", 1);
}

#
# Standard Testbed Header
#
PAGEHEADER("Confirm Verification");

#
# Grab the status and do the modification.
#
$status   = $this_user->status();
$wikionly = $this_user->wikionly();

#
# No multiple verifications.
# 
if (! strcmp($status, TBDB_USERSTATUS_ACTIVE) ||
    ! strcmp($status, TBDB_USERSTATUS_UNAPPROVED)) {
    USERERROR("You have already been verified. If you did not perform ".
	      "this verification, please notify Testbed Operations.", 1);
}

#
# The user is logged in, so all we need to do is confirm the key.
#
if ($key != $this_user->verify_key()) {
    USERERROR("The given key \"$key\" is incorrect. ".
	      "Please enter the correct key.", 1);
}

if ($status == TBDB_USERSTATUS_NEWUSER) {
    STARTBUSY("Verifying $uid");

    SUEXEC("nobody", "nobody", "webtbacct verify $uid", SUEXEC_ACTION_DIE);
    
    if ($wikionly) {
	#
	# For wikionly accounts, build the account now.
	# Just builds the wiki account of course (nothing else).
	#
	SUEXEC("nobody", "nobody", "webtbacct add $uid", SUEXEC_ACTION_DIE);

	STOPBUSY();

	#
	# The backend sets the actual WikiName
	#
	$this_user->Refresh();
	$wikiname = $this_user->wikiname();

	echo "<br>".
	    "You have been verified. You may now access the Wiki at<br>".
	    "<a href='$WIKIURL/$wikiname'>$WIKIURL/$wikiname</a>\n";
    }
    else {
	STOPBUSY();

	echo "<br>".
	     "You have now been verified. However, your application ".
	    "has not yet been approved. You will receive ".
	    "email when that has been done.\n";
    }
}
else {
    TBERROR("Bad user status '$status' for $uid!", 1);
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

