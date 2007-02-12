<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005, 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("node_defs.php");

#
# Only known and logged in users can do this.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$reqargs = RequiredPageArguments("node",      PAGEARG_NODE);
$optargs = OptionalPageArguments("linecount", PAGEARG_INTEGER);
$node_id = $node->node_id();

#
# Standard Testbed Header
#
PAGEHEADER("Console Log for $node_id");

if (!$isadmin &&
    !$node->AccessCheck($this_user, $TB_NODEACCESS_READINFO)) {
    USERERROR("You do not have permission to view the console!", 1);
}

$url = CreateURL("spewconlog", $node);

#
# Look for linecount argument
#
if (isset($linecount) && $linecount != "") {
    $url .= "&linecount=$linecount";
}

echo $node->PageHeader();
echo "<br><br>\n";

echo "<center>
      <iframe src='$url'
              width=90% height=500 scrolling=auto frameborder=1>
      Your user agent does not support frames or is currently configured
      not to display frames. However, you may visit
      <A href='$url'>the log file directly.</A>
      </iframe></center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
