<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2008 University of Utah and the Flux Group.
# All rights reserved.
#
require("defs.php3");

#
# Verify page arguments.
#
$optargs = OptionalPageArguments("printable",  PAGEARG_BOOLEAN);

#
# Allow for a site specific hardware page. Hopefully this is where the
# bulk of the site specific stuff can go, otherwise we need to get more
# clever about this.
#
$sitefile = "hardware-" . strtolower($THISHOMEBASE) . ".html";

if (!file_exists($sitefile)) {
    PAGEHEADER("Hardware Overview");
    USERERROR("This Emulab has not established a site-specific hardware page.",
	      1);
}

if (!isset($printable))
    $printable = 0;

#
# Standard Testbed Header
#
if (!$printable) {
    PAGEHEADER("Hardware Overview");
}

if (!$printable) {
    echo "<b><a href=$REQUEST_URI?printable=1>
             Printable version of this document</a></b><br>\n";
}

readfile("$sitefile");

#
# Standard Testbed Footer
# 
if (!$printable) {
    PAGEFOOTER();
}
?>

