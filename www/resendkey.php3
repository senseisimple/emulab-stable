<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Only known and logged in users can do this.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify form arguments.
# 
if (!isset($user) ||
    strcmp($user, "") == 0) {
    USERERROR("You must provide a User ID.", 1);
}

PAGEHEADER("Resend Verification Key");

if (!$isadmin) {
    USERERROR("You do not have permission to view this page!", 1);
}

#
# Confirm target is a real user.
#
if (! ($target_user = User::Lookup($user))) {
    USERERROR("The user $target_uid is not a valid user", 1);
}
$target_uid = $target_user->uid();

# Get email info and Key,
$usr_name  = $target_user->name();
$usr_email = $target_user->email();
$key       = $target_user->verify_key();

if (!$key || !strcmp($key, "")) {
    USERERROR("$target_uid does not have a valid verification key!", 1);
}

# Send the email.
TBMAIL("$usr_name '$target_uid' <$usr_email>",
       "Your New User Key",
       "\n".
       "Dear $usr_name ($target_uid):\n\n".
       "This is your account verification key: $key\n\n".
       "Please use this link to verify your user account:\n".
       "\n".
       "    ${TBBASE}/login.php3?vuid=$user&key=$key\n".
       "\n".
       "You will then be verified as a user.\n".
       "\n".
       "You MUST verify your account before any action can be taken on\n".
       "your application! After you have verified your account, and your\n".
       "application has been approved, you will be marked as an active\n".
       "user, and will be granted access to your user account.\n".
       "\n".
       "Thanks,\n".
       "Testbed Operations\n",
       "From: $TBMAIL_APPROVAL\n".
       "Bcc: $TBMAIL_AUDIT\n".
       "Errors-To: $TBMAIL_WWW");

echo "<center>
      <h2>Done!</h2>
      </center><br>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
