<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Login");

#
# Get the UID that came back in the cookie so that we can present a
# default login name to the user. If there is a UID from the browser,
# and the user is still logged in, then skip the form. Gotta log out
# first.
#
if (($known_uid = GETUID()) != FALSE) {
    if (CHECKLOGIN($known_uid) == $CHECKLOGIN_LOGGEDIN) {
	echo "<h3>
               You are still logged in. Please log out first if you want
               to log in as another user!
              </h3>\n";
	PAGEFOOTER();
	die("");
    }
}
else {
    $known_uid = "";
}

echo "<center><h3>
      Please login to our secure server.<br>
      (You must have cookies enabled)
      </h3></center>\n";

echo "<table align=center border=1>\n";
echo "<form action=\"${TBBASE}/login.php3\" method=post>\n";
echo "<tr>
         <td>Username:</td>
         <td><input type=text value=\"$known_uid\" name=uid size=8></td>
      </tr>
      <tr>
         <td>Password:</td>
         <td><input type=password name=password size=12></td>
      </tr>
      <tr>
         <td align=center colspan=2>
             <b><input type=submit value=Login name=login></b></td>
      </tr>\n";
echo "</form>\n";
echo "</table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>






