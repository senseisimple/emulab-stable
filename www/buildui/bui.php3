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

#if (!$isadmin) {
#    USERERROR("You do not have permission to use this interface!", 1);
#}

chdir("buildui");

echo "<applet code=\"Netbuild.class\" width=800 height=600>
       <param name=exporturl
              value=\"$TBBASE/beginexp.php3\">
       <pre>
         The applet should be right here. Must be an error or something.
       </pre>
      </applet>\n";
?>
<hr>
<h2>Basic usage:</h2>
<list>
<li>
  <p>Drag Nodes and LANs from the <i>Palette</i> on the left into the <i>Workarea</i> in the middle.</p>
</li>
<li>
  <p>To link a Node to a Node (or to a LAN,) select the node (by clicking it,) then hold "ctrl" and click on the node (or LAN) you wish to link it to.</p> 
</li>
<li>
  <p>Clicking the "create experiment" button will send you to the Emulab "create experiment" web page, automatically generating and sending an NS file for your designed topology along. From that page, you may create the experiment and/or view the generated NS file.</p>
</li>
</list>
<p>
<a href="../doc/docwrapper.php3?docname=netbuilddoc.html">Netbuild Full Reference</a>
</p>

<?php
#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
