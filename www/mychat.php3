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
      <br><b>$password</b></center><br>\n";
echo "<br>\n";
echo "The Emulab <a href=http://jabberd.jabberstudio.org/2/>Jabber</a>
      server is an implementation of the popular
      <a href=http://www.jabber.org/>Jabber Instant Messaging Protocol.</a>
      You can use your own Jabber client, or you can use the Jeti Java
      Applet by clicking on your Jabber ID above.\n";
echo "<br><br>\n";
echo "If you decide to use your own Jabber client (which we recommend), then
      you should check out <a href=http://gaim.sourceforge.net/>Gaim</a>, 
      a multi-protocol instant messaging (IM) client that many on the Emulab
      development team use. Should you decide to use Gaim, here are the
      relevant fields in the <b>Add Account</b> screen (you can check out
      <a href=http://www.google.com/talk/otherclients.html>Google's Gaim</a>
      tutorial to see how to get to the Add Accounts screen).\n";

echo "<ul>
       <li><b>Protocol</b>: Jabber
       <li><b>Screen Name</b>: your Emulab user ID
       <li><b>Server</b>: jabber.${OURDOMAIN}
       <li><b>Password</b>: your Emulab jabber password (see above)
       <li><b>Protocol</b>: Jabber
       <li><b>Jabber Options</b>: 'Use TLS if available'
       <li><b>Port</b>: 5222
       <li><b>Connect server</b>: leave this field blank
       <li><b>Proxy type</b>: Use Global Proxy Settings
      </ul>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
