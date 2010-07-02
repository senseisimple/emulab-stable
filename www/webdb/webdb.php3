<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002, 2006 University of Utah and the Flux Group.
# All rights reserved.
#

chdir("..");
require("defs.php3");

#
# Only known and logged in users can do this.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();
$dbedit    = $this_user->dbedit();

if (! $dbedit) {
    USERERROR("You do not have permission to use WEBDB!", 1);
}

header("Pragma: no-cache");
echo "<html>
      <head>\n";

chdir("webdb");

include "webdb_backend.php3";

webdb_backend_main();

?>    
    <title>WebDb - <?php echo $title_header ?></title>

    <style type="text/css"><!--
      p.location {
	color: #11bb33;
	font-size: small;
      }
      body {
	background-color: #EEFFEE;
      }
      h1 {
	color: #004400;
      }
      th {
	background-color: #AABBAA;
	color: #000000;
	font-size: x-small;
      }
      td {
	background-color: #D4E5D4;
	font-size: x-small;
      }
      form {
	margin-top: 0;
	margin-bottom: 0;
      }
      a {
	text-decoration:none;
	color: #248200;
      }
      a:link {}
      a:hover {
	ba2ckground-color:#EEEFD5;
	color:#54A232;
	text-decoration:underline;               
      }
      //-->
 </style>


  </head>
  <body>
    <h1><?php echo $body_header ?></h1> 
    <?php echo $body?>
    <hr><p>based on mysql.php3 by SooMin Kim.</p>
  </body>
</html>










