<?php
chdir("..");
require("defs.php3");

PAGEHEADER("NetBuild");

#
# Only known and logged in users can do this.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

if (!$isadmin) {
    USERERROR("You do not have permission to use this interface!", 1);
}

chdir("buildui");

echo "<applet code=\"Netbuild.class\" width=640 height=480>
       <param name=exporturl
              value=\"$TBBASE/beginexp.php3\">
       <pre>
         The applet should be right here. Must be an error or something.
       </pre>
      </applet>\n";
?>
<hr>
<p>
<a href="../doc/docwrapper.php3?docname=netbuilddoc.html">How to use Netbuild</a>
</p>

<?php
#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
