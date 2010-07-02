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
$reqargs = RequiredPageArguments("docname",    PAGEARG_STRING);
$optargs = OptionalPageArguments("printable",  PAGEARG_BOOLEAN);

#
# Need to sanity check the path! Allow only [word].html files
#
if (!preg_match("/^[-\w]+\.(html|txt)$/", $docname)) {
    USERERROR("Illegal document name: $docname!", 1, HTTP_400_BAD_REQUEST);
}

#
# Make sure the file exists
#
$fh = @fopen("$docname", "r");
if (!$fh) {
    USERERROR("Can't read document file: $docname!", 1, HTTP_404_NOT_FOUND);
}

if (!isset($printable))
    $printable = 0;

#
# Standard Testbed Header
#
if (!$printable) {
    PAGEHEADER("Emulab Hyperviewer");
}

if ($printable) {
    #
    # Need to spit out some header stuff.
    #
    echo "<html>
          <head>
  	  <link rel='stylesheet' href='../tbstyle-plain.css' type='text/css'>
          </head>
          <body>\n";
}
else {
	echo "<b><a href=$REQUEST_URI&printable=1>
                 Printable version of this document</a></b><br>\n";
}

fpassthru($fh);
fclose($fh);

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

