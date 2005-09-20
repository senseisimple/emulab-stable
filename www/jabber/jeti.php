<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2005 University of Utah and the Flux Group.
# All rights reserved.
#

$user	  = $_GET['user'];
$password = $_GET['password'];

echo "<html>
      <head>
        <title>Jeti Applet</title>
      </head>
      <body>
        <applet name=jeti archive=\"applet.jar,plugins/alertwindow.jar,".
           "plugins/emoticons.jar,plugins/groupchat.jar,".
           "plugins/appletloadgroupchat.jar,plugins/sound.jar,".
           "plugins/xhtml.jar\" ".
               "codebase=/jeti ".
               "code=nu.fw.jeti.applet.Jeti.class
                width=50% height=50%>
         <param name=server value=jabber.emulab.net>
         <param name=password value=$password> 
         <param name=user value=$user> 
         <param name=port value=5223> 
         <param name=ssl value=true>
        </applet>
        </body>
        </html>\n";

?>
