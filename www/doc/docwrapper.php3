<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
chdir("..");
require("defs.php3");
chdir("doc");

#
# Verify page arguments.
#
$reqargs = RequiredPageArguments("docname",    PAGEARG_STRING);
$optargs = OptionalPageArguments("printable",  PAGEARG_BOOLEAN);

#
# Need to sanity check the path! Allow only [word].{html,txt} files
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
    PAGEHEADER("Emulab Documentation");
}

#
# Check extension. If a .txt file, need some extra wrapper stuff to make
# it look readable.
#
$textfile = 0;
if (preg_match("/^.*\.txt$/", $docname)) {
    $textfile = 1;
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

if ($textfile) {
    echo "<XMP>\n";
}

fpassthru($fh);
fclose($fh);

if ($textfile) {
    echo "</XMP>\n";
}

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






