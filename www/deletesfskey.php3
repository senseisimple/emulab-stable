<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# No PAGEHEADER since we spit out a redirect later.
#

#
# Only known and logged in users can do this.
#
$this_user = CheckLoginOrDie(CHECKLOGIN_USERSTATUS|CHECKLOGIN_WEBONLY);
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$reqargs = RequiredPageArguments("target_user", PAGEARG_USER,
				 "key",         PAGEARG_STRING);
$optargs = OptionalPageArguments("canceled",    PAGEARG_BOOLEAN,
				 "confirmed",   PAGEARG_BOOLEAN);

# Pedantic argument checking of the key.
if (! preg_match("/^[-\w\.\@\#]+$/", $key)) {
    PAGEARGERROR();
}

# Need these below
$target_uid   = $target_user->uid();

#
# Verify that this uid is a member of one of the projects that the
# target_uid is in. Must have proper permission in that group too. 
#
if (!$isadmin && 
    !$target_user->AccessCheck($this_user, $TB_USERINFO_MODIFYINFO)) {
    USERERROR("You do not have permission!", 1);
}

#
# Get the actual key.
#
$query_result =& $target_user->TableLookUp("user_sfskeys",
					   "pubkey,comment",
					   "comment='$key'");

if (! mysql_num_rows($query_result)) {
    USERERROR("SFS Key '$key' for user '$target_uid' does not exist!", 1);
}

$row    = mysql_fetch_array($query_result);
$pubkey = $row['pubkey'];
$comment= $row['comment'];
$chunky = chunk_split("$pubkey $comment", 70, "<br>\n");

#
# We run this twice. The first time we are checking for a confirmation
# by putting up a form. The next time through the confirmation will be
# set. Or, the user can hit the cancel button, in which case we should
# probably redirect the browser back up a level.
#
if (isset($canceled) && $canceled) {
    PAGEHEADER("SFS Public Key Maintenance");
    
    echo "<center><h2><br>
          SFS Public Key deletion canceled!
          </h2></center>\n";

    $url = CreateURL("deletesfskey", $target_user);

    echo "<br>
          Back to <a href='$url'>sfs public keys</a> for user '$uid'.\n";
    
    PAGEFOOTER();
    return;
}

if (!isset($confirmed)) {
    PAGEHEADER("SFS Public Key Maintenance");

    echo "<center><h3><br>
          Are you <b>REALLY</b>
          sure you want to delete this SFS Public Key for user '$target_uid'?
          </h3>\n";

    $url = CreateURL("deletesfskey", $target_user, "key", $key);
    
    echo "<form action='$url' method=post>";
    echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
    echo "</form>\n";
    echo "</center>\n";

    echo "<table align=center border=1 cellpadding=2 cellspacing=2>
           <tr>
              <td>$chunky</td>
           </tr>
          </table>\n";
    
    PAGEFOOTER();
    return;
}

#
# Audit
#
$uid_name  = $this_user->name();
$uid_email = $this_user->email();

$targuid_name  = $target_user->name();
$targuid_email = $target_user->email();

TBMAIL("$targuid_name <$targuid_email>",
     "SFS Public Key for '$target_uid' Deleted",
     "\n".
     "SFS Public Key for '$target_uid' deleted by '$uid'.\n".
     "\n".
     "$chunky\n".
     "\n".
     "Thanks,\n".
     "Testbed Operations\n",
     "From: $uid_name <$uid_email>\n".
     "Bcc: $TBMAIL_AUDIT\n".
     "Errors-To: $TBMAIL_WWW");

$target_user->TableDelete("user_sfskeys", "comment='$key'");

#
# update sfs_users files and nodes if appropriate.
#
if (HASREALACCOUNT($uid)) {
    SUEXEC($uid, "nobody", "webaddsfskey -w $target_uid", 0);
}
else {
    SUEXEC("nobody", "nobody", "webaddsfskey -w $target_uid", 0);
}

PAGEREPLACE(CreateURL("showsfskeys", $target_user));

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
