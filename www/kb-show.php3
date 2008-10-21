<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2008 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

# Page arguments.
$optargs = OptionalPageArguments("idx",       PAGEARG_INTEGER,
				 "xref_tag",  PAGEARG_STRING);

if (isset($xref_tag) && $xref_tag != "") {
    if (! preg_match("/^[-\w]+$/", $xref_tag)) {
	PAGEARGERROR("Invalid characters in $xref_tag");
    }
    $query_result =
	DBQueryFatal("select * from knowledge_base_entries ".
		     "where xref_tag='$xref_tag'");
    if (! mysql_num_rows($query_result)) {
	USERERROR("No such knowledge_base entry: $xref_tag", 1, 
		  HTTP_404_NOT_FOUND);
    }
    $row = mysql_fetch_array($query_result);
    $idx = $row['idx'];
}
if (isset($idx)) {
    header("Location: $WIKIDOCURL/kb${idx}", TRUE, 301);
}
else {
    header("Location: $WIKIDOCURL/KnowledgeBase", TRUE, 301);
}    
