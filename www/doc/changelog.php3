<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005 University of Utah and the Flux Group.
# All rights reserved.
#
chdir("..");
require("defs.php3");
chdir("doc");

PAGEHEADER("Changelog/Technical Details");

echo "<center>
        <a href='docwrapper.php3?docname=ChangeLog.txt'>
              <font size=+1>One <b>Big</b> File</font></a>
      <br>
      <br>
      Or<br>
      Month by Month<br>\n";
      
echo "<table align=center class=stealth border=0>\n";
readfile("changelog-months.html");
echo "</table>
      <br />\n";

PAGEFOOTER();
?>

