<?php
#
# Beware empty spaces (cookies)!
#
# The point of this file is so that when people go to www.emulab.net 
# they will be redirected from http://www.emulab.net/index.html to
# https://www.emulab.net/start.php3, so that we can force certain traffic
# through the secure server instead of the plain server. $TBBASE sets the
# base pointer for the secure server. The problem though, is that we
# don't want the documentation to go through the secure server, so
# specify the nonsecure server for the welcome frame using $TBDOCBASE.
# 
require("defs.php3");

echo "<html>
         <head>
            <title>Emulab.Net</title>
	    <base href=\"$TBBASE/\">
         </head>
         <frameset cols=\"135,*\">
            <frame src=\"index.php3\" name=\"fixed\">
            <frame src=\"welcome.html\" name=\"dynamic\">
         </frameset>
     </html>\n";
?>

