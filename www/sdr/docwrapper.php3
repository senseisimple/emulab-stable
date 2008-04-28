<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
require("usrpdefs.php3");

#
# Verify page arguments.
#
$reqargs = RequiredPageArguments("docname",    PAGEARG_STRING);
$optargs = OptionalPageArguments("printable",  PAGEARG_BOOLEAN,
				 "title",      PAGEARG_STRING);

#
# Need to sanity check the path! Allow only [word].{html,txt} files
#
if (!preg_match("/^[-\w]+\.(html|txt)$/", $docname)) {
    header(' ', true, 400);
    USERERROR("Illegal document name: $docname!", 1);
}

#
# Make sure the file exists
#
$fh = @fopen("$docname", "r");
if (!$fh) {
    header(' ', true, 404);
    USERERROR("Can't read document file: $docname!", 1);
}

if (!isset($printable))
    $printable = 0;
if (!isset($title) || $title == "")
     $title = "Emulab Documentation";

#
# Standard Testbed Header
#
if (!$printable) {
    PAGEHEADER($title, $USRP_MENUDEFS);
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

