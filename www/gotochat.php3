<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2005 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

if (!$CHATSUPPORT) {
    header("Location: index.php3");
    return;
}

# No Pageheader since we spit out a redirection below.
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

$query_result =
    DBQueryFatal("select mailman_password from users where uid='$uid'");

if (!mysql_num_rows($query_result)) {
    USERERROR("No such user $uid!", 1);
}
$row = mysql_fetch_array($query_result);
$password = $row['mailman_password'];
$url = "${OPSJETIURL}?user=$uid&password=$password";

header("Location: ${url}");
?>
