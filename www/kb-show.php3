<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

# Page arguments.
$printable = $_GET['printable'];
if (!isset($printable))
    $printable = 0;

#
# Standard Testbed Header
#
if (!$printable) {
    PAGEHEADER("Knowledge Base Entry");
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
    echo "<b><a href=$REQUEST_URI&printable=1>
                Printable version of this document</a></b><br>\n";
}

#
# Admin users get a menu.
#
if (($this_user = CheckLogin($ignore))) {
    $isadmin = ISADMIN();
}
else {
    $isadmin = 0;
}

#
# Check record.
# 
if (isset($idx) && $idx != "") {
    if (! TBvalid_integer($idx)) {
	PAGEARGERROR("Invalid characters in $idx");
    }
    $query_result =
	DBQueryFatal("select * from knowledge_base_entries ".
		     "where idx='$idx'");
    if (! mysql_num_rows($query_result)) {
	USERERROR("No such knowledge_base entry: $idx", 1);
    }
    $row = mysql_fetch_array($query_result);
    $xref_tag = $row['xref_tag'];
}
elseif (isset($xref_tag) && $xref_tag != "") {
    if (! preg_match("/^[-\w]+$/", $xref_tag)) {
	PAGEARGERROR("Invalid characters in $xref_tag");
    }
    $query_result =
	DBQueryFatal("select * from knowledge_base_entries ".
		     "where xref_tag='$xref_tag'");
    if (! mysql_num_rows($query_result)) {
	USERERROR("No such knowledge_base entry: $xref_tag", 1);
    }
    $row = mysql_fetch_array($query_result);
    $idx = $row['idx'];
}

else {
    PAGEARGERROR("Must supply a knowledge_base index or xref tag");
}


echo "<center><b>Knowledge Base Entry: $idx $xref_tag</b><br>".
     "(<a href=kb-search.php3>Search Again</a>)</center>\n";
echo "<br><br>\n";

if (!$printable && $isadmin) {
    SUBPAGESTART();
    SUBMENUSTART("Options");
    
    WRITESUBMENUBUTTON("Modify Entry",
		       "kb-manage.php3?idx=$idx&action=edit");
    WRITESUBMENUBUTTON("Delete Entry",
		       "kb-manage.php3?idx=$idx&action=delete");
    WRITESUBMENUBUTTON("Add Entry", "kb-manage.php3");
    SUBMENUEND();
}

echo "<blockquote>\n";
echo "<font size=+1>" . $row['title'] . "</font>\n";
echo "<br><br>\n";
echo $row['body'];

echo "<br><br>\n";
echo "<font size=-2>";
echo "Posted by " . $row['creator_uid'] . " on " . $row['date_created'];
if (isset($row['date_modified'])) {
    echo ", ";
    echo "Modified by " . $row['modifier_uid'] . " on " .$row['date_modified'];
}
echo "</font><br>\n";

#
# Get other similar topics and list the titles.
#
if (!$printable) {
    $query_result =
	DBQueryFatal("select * from knowledge_base_entries ".
		     "where section='". $row['section'] . "'");

    if (mysql_num_rows($query_result)) {
	echo "<hr>";
	echo "<b>Other similar topics</b>:<br>\n";
	echo "<blockquote>\n";
	echo "<ul>\n";

	while ($row = mysql_fetch_array($query_result)) {
	    $title    = $row['title'];
	    $idx      = $row['idx'];
    
	    echo "<li>";
	    echo "<a href=kb-show.php3?idx=$idx>$title</a>\n";
	}
	echo "</ul>\n";
	echo "</blockquote>\n";
    }
}

if (!$printable && $isadmin) {
    echo "</blockquote>\n";
    SUBPAGEEND();
}

if ($printable) {
    echo "</body>
          </html>\n";
}
else {
    PAGEFOOTER();
}
?>
