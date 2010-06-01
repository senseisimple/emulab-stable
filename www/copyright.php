<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2010 University of Utah and the Flux Group.
# All rights reserved.
#
require("defs.php3");

#
# Verify page arguments.
#
$optargs = OptionalPageArguments("printable",  PAGEARG_BOOLEAN);

if (!isset($printable))
    $printable = 0;

#
# Standard Testbed Header
#
if (!$printable) {
    PAGEHEADER("Emulab Copyright Notice");
}

if ($printable) {
    #
    # Need to spit out some header stuff.
    #
    echo "<html>
          <head>
          <title>Copyright Notice</title>
  	  <link rel='stylesheet' href='tbstyle-plain.css' type='text/css'>
          </head>
          <body>\n";
}
else {
    echo "<b><a href='copyright.php?printable=1'>
              Printable version of this document</a></b><br>\n";

    echo "<p><center><b>Copyright Notice</b></center><br>\n";
}

#
# Allow for a site specific copyright
#
$sitefile = "copyright-local.html";

if (!file_exists($sitefile)) {
    $sitefile = "copyright-standard.html";
}
readfile("$sitefile");

#
# Standard Testbed Footer
# 
if ($printable) {
    echo "</body>
          </html>\n";
}
else {
    PAGEFOOTER();
}

?>
