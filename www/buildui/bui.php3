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

?>
<applet code="Netbuild.class" width=640 height=480>
        <param name="exporturl"
               value="https://www.mini.emulab.net/beginexp_form.php3">
        <pre>The applet should be right here. Must be an error or
             something.</pre>
</applet>

<pre>
Helpful Hints:

To create nodes, drag them from the bar on the left into the work area
in the middle.

To link all currently selected node(s)/lan(s) to another node/lan, hold
ctrl and click on that node/lan.

To select multiple items, use a sweeping rectangle or hold shift (or
both.) Use this to change parameters for a large group of things at once.
</pre>

<?php
#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
