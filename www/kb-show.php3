<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2005 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Knowledge Base Entry");

#
# Admin users get a menu.
#
$uid     = GETLOGIN();
$isadmin = 0;
if (CHECKLOGIN($uid) & CHECKLOGIN_LOGGEDIN) {
    $isadmin = ISADMIN();
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

echo "<center><b>Knowledge Base Entry: $idx</b><br>".
     "(<a href=kb-search.php3>Search Again</a>)</center>\n";
echo "<br><br>\n";

if ($isadmin) {
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

echo "<br><br><br><br>\n";
echo "<font size=-2>Posted by " . $row['creator_uid'] . " on " .
      $row['date_created'] . "</font><br><br>\n";

if ($isadmin) {
    echo "</blockquote>\n";
    SUBPAGEEND();
}

PAGEFOOTER();
?>
