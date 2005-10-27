<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005 University of Utah and the Flux Group.
# All rights reserved.
#
require("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Emulab Knowledge Base");

#
# Get all entries.
# 
$search_result =
    DBQueryFatal("select * from knowledge_base_entries ".
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
PAGEFOOTER();
?>

