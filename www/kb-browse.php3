<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005, 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
require("defs.php3");

$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();
$optargs   = OptionalPageArguments("printable", PAGEARG_BOOLEAN);

# Some Knowledge Base entries are visible only to admins.
$admin_access = $isadmin || ISFOREIGN_ADMIN();

if (!isset($printable))
    $printable = 0;

#
# Standard Testbed Header
#
if (!$printable) {
    PAGEHEADER("Emulab Knowledge Base");
}

if ($printable) {
    #
    # Need to spit out some header stuff.
    #
    echo "<html>
          <head>
  	  <link rel='stylesheet' href='tbstyle-plain.css' type='text/css'>
          </head>
          <body>\n";
}
else {
    echo "<b><a href=$REQUEST_URI?printable=1>
                Printable version of this document</a></b><br>\n";
}

#
# Get all entries.  Only admins see section='Testbed Operations'.
# 
$search_result =
    DBQueryFatal("select * from knowledge_base_entries ".
		 ($admin_access ? "" : 
		  "where section != 'Testbed Operations' ").
		 "order by section,date_created");

if (! mysql_num_rows($search_result)) {
    USERERROR("There are no entries in the Emulab Knowledge Base!", 1);
    return;
}

echo "<center><h2>Browse Emulab Knowledge Base</h2></center>\n";
echo "<ul>\n";

#
# First the table of contents.
#
$lastsection = "";

while ($row = mysql_fetch_array($search_result)) {
    $section  = $row['section'];
    $title    = $row['title'];
    $idx      = $row['idx'];

    if ($lastsection != $section) {
	if ($lastsection != "") {
	    echo "</ul><hr>\n";
	}
	$lastsection = $section;
	
	echo "<li><font size=+1><b>$section</b></font>\n";
	echo "<ul>\n";
    }
    echo "<li>";
    echo "<a href=#${idx}>$title</a>\n";
}
mysql_data_seek($search_result, 0);

echo "</ul></ul>\n";
echo "<hr size=4>\n";
echo "<ul>\n";

$lastsection = "";

while ($row = mysql_fetch_array($search_result)) {
    $section  = $row['section'];
    $title    = $row['title'];
    $body     = $row['body'];
    $idx      = $row['idx'];
    $xref_tag = $row['xref_tag'];

    if ($lastsection != $section) {
	if ($lastsection != "") {
	    echo "</ul><hr>\n";
	}
	$lastsection = $section;
	
	echo "<li><font size=+1><b>$section</b></font>\n";
	echo "<ul>\n";
    }
    echo "<li>";
    if (isset($xref_tag) && $xref_tag != "") {
	echo "<a NAME='$xref_tag'></a>";
    }
    echo "<a NAME='$idx'></a>";
    echo "<a href=kb-show.php3?idx=$idx>$title</a>\n";

    echo "<blockquote>\n";
    echo $body;
    echo "<br>\n";
    echo "</blockquote>\n";
}

echo "</ul></ul>\n";

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

