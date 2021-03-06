<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2004, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
chdir("..");
require("defs.php3");
chdir("hyperviewer");

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
    PAGEHEADER("Emulab Hyperviewer");
}

if (!$printable) {
    echo "<b><a href=$REQUEST_URI?printable=1>
             Printable version of this document</a></b><br>\n";
}

readfile("index.html");

#
# Standard Testbed Footer
# 
if (!$printable) {
    PAGEFOOTER();
}
?>

