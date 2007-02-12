<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Page Not Working Properly!");

$optargs = OptionalPageArguments("confirmed", PAGEARG_BOOLEAN,
				 "referrer",  PAGEARG_STRING);

$this_user = CheckLogin($check_status);
$referrer  = (isset($referrer) ? $referrer : $_SERVER['HTTP_REFERER']);

if ($this_user) {
    if (!isset($confirmed)) {
	echo "Are you sure you want to report that something is wrong with
              <b>$referrer</b><br><br>";

	echo "<center>\n";
 	echo "<form action='pagenotworking.php' method=get>";
	echo "<b><input type=hidden name=confirmed value=1></b>";
	echo "<b><input type=hidden name=referrer value='$referrer'></b>";
	echo "<b><input type=submit name=tag value='Submit'></b>";
	echo "</form>";
	echo "</center>\n";
    }
    else {
	$uid_name  = $this_user->name();
	$uid_email = $this_user->email();
	$uid_uid   = $this_user->uid();

	TBMAIL($TBMAIL_OPS,
	       "Page Not Working Properly",
	       "$uid_name ($uid_uid) is reporting that page:\n\n".
	       "    $referrer\n\n".
	       "is not working properly",
	       "From: $uid_name <$uid_email>\n".
	       "Errors-To: $TBMAIL_WWW");

	echo "<br>
         Thanks! A message has been sent to $TBMAILADDR to let us know
         something might be wrong with <b>$referrer</b>";
    }
}

echo "<br><br><br>\n";
echo "Back to <a href='$referrer'>previous page</a>\n";
echo "<br>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
