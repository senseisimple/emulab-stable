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

#
# Only known and logged in users.
#
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

PAGEHEADER("My Instant Messaging");

echo "<center><font size=+1>Your Emulab Jabber ID and Password</font>
      </center><br>\n";
echo "<center><a href=gotochat.php3><b>$uid@jabber.{$OURDOMAIN}</b></a>
      <br><b>$password</b>
      </center>\n";
echo "<br><br>\n";
echo "The Emulab <a href=http://jabberd.jabberstudio.org/2/>Jabber</a>
      server is an implementation of the popular
      <a href=http://www.jabber.org/>Jabber Instant Messaging Protocol.</a>
      You can use your own Jabber client, or you can use the Jeti Java
      Applet by clicking on your Jabber ID above.\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
