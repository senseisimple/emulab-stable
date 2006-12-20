<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002, 2005, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("New User Verification");

#
# Only known and logged in users can be verified.
#
$this_user = CheckLoginOrDie(CHECKLOGIN_UNVERIFIED|CHECKLOGIN_NEWUSER|
			     CHECKLOGIN_WEBONLY|CHECKLOGIN_WIKIONLY);
$uid       = $this_user->uid();

echo "<p>
      The purpose of this page is to verify, for security purposes, that
      information given in your application is authentic. If you never
      received a key at the email address given on your application, please
      contact $TBMAILADDR for further assistance.
      <p>\n";

echo "<table align=\"center\" border=\"1\">
      <form action=\"verifyusr.php3\" method=\"post\">\n";

echo "<tr>
          <td>Key:</td>
          <td><input type=\"text\" name=\"key\" size=20></td>
      </tr>\n";

echo "<tr>
         <td colspan=\"2\" align=\"center\">
             <b><input type=\"submit\" value=\"Submit\"></b></td>
      </tr>\n";

echo "</form>\n";
echo "</table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>




