<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
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
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

$password = $this_user->mailman_password();

PAGEHEADER("My Instant Messaging");

echo "<center><font size=+1>Your Emulab Jabber ID and Password</font>
      </center><br>\n";
echo "<center><a href=gotochat.php3><b>$uid@jabber.{$OURDOMAIN}</b></a>
      <br><b>$password</b></center><br>\n";
echo "<br>\n";
echo "The Emulab <a href=http://jabberd.jabberstudio.org/2/>Jabber</a>
      server is an implementation of the popular
      <a href=http://www.jabber.org/>Jabber Instant Messaging Protocol.</a>
      You can use your own Jabber client, or you can use the <b>Jeti Java
      Applet by clicking on your Jabber ID above</b>.\n";

echo "<br><br>
      Once you connect to Emulab's Jabber server, you will find an initial
      set of buddy lists, one for each project you belong to. Emulab will
      maintain those buddy lists for you, adding and removing project members
      as needed. Of course you are welcome to add your own buddies and buddy
      lists; Emulab will not interfere with them.\n";

echo "<br>
      The Emulab Jabber server also supports multi user <em>chatrooms</em>.
      You are welcome to create chatrooms as needed. Simply use
      <tt>conference.emulab.net</tt> as the server hostname when prompted. ";
echo "<em>(We plan to add automatic creation of per-project chatrooms in
      the future)</em>\n";

echo "<br><br>\n";
echo "If you decide to use your own Jabber client (which we recommend), then
      you should check out <a href=http://gaim.sourceforge.net/>Gaim</a>, 
      a multi-protocol instant messaging (IM) client that many on the Emulab
      development team use. Should you decide to use Gaim, here are the
      relevant fields in the <b>Add Account</b> screen (you can check out
      <a href=http://www.google.com/talk/otherclients.html>Google's Gaim</a>
      tutorial to see how to get to the Add Accounts screen).\n";

echo "<blockquote><ul>
       <li><b>Protocol</b>: Jabber
       <li><b>Screen Name</b>: your Emulab user ID
       <li><b>Server</b>: jabber.${OURDOMAIN}
       <li><b>Password</b>: your Emulab jabber password (see above)
       <li><b>Protocol</b>: Jabber
       <li><b>Jabber Options</b>: 'Use TLS if available'
       <li><b>Port</b>: 5222
       <li><b>Connect server</b>: leave this field blank
       <li><b>Proxy type</b>: Use Global Proxy Settings
      </ul></blockquote>\n";

echo "Of course, any Jabber compatible IM client can be used. The Google
      page mentioned above has a nice list of clients, along with
      instructions on how to configure them all. We suggest you use one of
      those clients.\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
