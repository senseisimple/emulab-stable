<?php
include("defs.php3");

#
# No PAGEHEADER since we spit out a Location header later. See below.
# 

#
# Only known and logged in users can do this.
#
# Note different test though, since we want to allow logged in
# users with expired passwords to change them.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Admins can change status for other users.
#
if (!isset($target_uid)) {
    $target_uid = $uid;
}

#
# We need to know the real admin permission of the current user.
#
if (! ($CHECKLOGIN_STATUS & CHECKLOGIN_ISADMIN)) {
    USERERROR("You do not have permission to use this page!", 1);
}

if (!isset($adminoff) || ($adminoff != 0 && $adminoff != 1)) {
    USERERROR("Improper arguments!", 1);
}

DBQueryFatal("update users set adminoff=$adminoff where uid='$target_uid'");

#
# Spit out a redirect 
# 
header("Location: $TBBASE/showuser.php3?target_uid=$target_uid");

?>
