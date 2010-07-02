<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005, 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
if (!isset($embedded)) {
    require("defs.php3");

    # Some Knowledge Base entries are visible only to admins.
    $this_user = CheckLogin($check_status);
    $admin_access = ISADMIN() || ISFOREIGN_ADMIN();
    $embedded = 0;

    $optargs = OptionalPageArguments("submit",      PAGEARG_STRING,
				     "query",       PAGEARG_STRING,
				     "query_which", PAGEARG_STRING,
				     "query_type",  PAGEARG_STRING);
}
else {
    #
    # Sheesh. This file is included from search.php3, hence the sillyness.
    #
    $embedded = 1;
    $admin_access = 0;
}

#
# Standard Testbed Header
#
if (!$embedded) {
    PAGEHEADER("Search Emulab Knowledge Base");
} 

function SPITFORM($query, $query_type, $query_which, $error)
{
    global $TBBASE;

    echo "<table align=center border=1>
          <form action=kb-search.php3 method=get>\n";

    $query = htmlspecialchars($query);

    if ($error) {
	echo "<center><font color=red>
	      $error
	      </font></center><br>\n";
    }

    #
    # The query
    #
    echo "<tr>
             <td>Keywords:</td>
             <td class=left>
                 <input type=text name=query value=\"$query\"
                        size=50 maxlength=100>
             </td>
           </tr>\n";

    #
    # The query type
    #
    $temp_array = array("and"   => "All Words",
			"or"    => "Any Words",
			"exact" => "Exact Phrase");
    if (!$query_type)
	$query_type = "and";
    
    echo "<tr>
             <td>Search for:</td>
             <td class=left>\n";

    foreach ($temp_array as $key => $phrase) {
	$checked = "";

	if ($query_type == $key)
	    $checked = "checked";
	
	echo "<input type=radio $checked name=query_type value=$key>$phrase\n";
    }
    echo "   </td>
           </tr>\n";

    #
    # What to search
    #
    $temp_array = array("title" => "Title",
			"body"  => "Body",
			"both"  => "Both");
    if (!$query_which)
	$query_which = "title";

    echo "<tr>
             <td>Search what:</td>
             <td class=left>\n";

    foreach ($temp_array as $key => $phrase) {
	$checked = "";

	if ($query_which == $key)
	    $checked = "checked";
	
	echo "<input type=radio $checked name=query_which value=$key>".
	    "$phrase\n";
    }
    echo "   </td>
          </tr>\n";

    echo "<tr>
              <td colspan=2 align=center>
                 <b><input type=submit name=submit value='Submit Query'></b>
              </td>
          </tr>\n";

    echo "</form>
          </table><br>\n";

    # Link to kb-browse through $TBBASE (https:), not $TBDOCBASE (http:).
    # On https:, the browser sends HashCookie, so we get CHECKLOGIN_LOGGEDIN
    # status.  Going via http:, we get CHECKLOGIN_MAYBEVALID, and can't know
    # whether to show admin KB entries.
    echo "<center>".
	 "You may also <a href=$TBBASE/kb-browse.php3>
	     browse the entire Knowledge Base</a>.".
         "</center>\n";
}

#
# First page load ...
# 
if (!isset($submit) && !$embedded) {
    SPITFORM("", null, "both", null);
    PAGEFOOTER();
    return;
}

#
# Check the query type
#
if (!isset($query_type) || $query_type == "") {
    $query_type == "and";
}
if (! ($query_type == "and" || $query_type == "or" ||
       $query_type == "exact")) {
    PAGEARGERROR("Improper query type $query_type");
}

#
# Check the query which
#
if (!isset($query_which) || $query_which == "") {
    $query_which == "title";
}
if (! ($query_which == "title" ||
       $query_which == "body" || $query_which == "both")) {
    PAGEARGERROR("Improper query which $query_which");
}
# Strip leading and trailing whitespace to avoid confusion below.
$query = trim($query);

#
# Must supply a query!
# 
if (!isset($query) || $query == "") {
    SPITFORM("", $query_type, $query_which, "Please provide a query!");
    PAGEFOOTER();
    return;
}

#
# Check the query
#
if (! TBvalid_userdata($query)) {
    SPITFORM($query, $query_type, $query_which, "Illegal characters in query");
    PAGEFOOTER();
    return;
}

#
# Look for special "*" query; just get everything and list it. 
#
if ($query == "*" ||
    preg_match("/^\s+$/", $query)) {
    $search_result =
	DBQueryFatal("select * from knowledge_base_entries ".
		     ($admin_access ? "" :
		      "where section != 'Testbed Operations' ").
		     "order by section,date_created");
}
else {
    #
    # Mysql 4.0 has all this stuff built in, but not 3.23. So, do it by hand.
    #
    #
    # Exact phrase search is easy!
    #
    if ($query_type == "exact") {
	$clause = "";
	$qsafe  = addslashes($query);

	if ($query_which == "title") {
	    $clause = "where title like '%${qsafe}%'";
	}
	elseif ($query_which == "body") {
	    $clause = "where body like '%${qsafe}%'";
	}
	elseif ($query_which == "both") {
	    $clause = "where body like '%${qsafe}%' ".
		"or title like '%${qsafe}%'";
	}
    }
    elseif ($query_type == "or") {
	$wordarray = preg_split("/[\s,]+/", $query);

	foreach ($wordarray as $i => $word) {
	    $wordarray[$i] = addslashes($word);
	}
	$qstring = implode("|", $wordarray);
	    
	if ($query_which == "title") {
	    $clause = "where title regexp '$qstring'";
	}
	elseif ($query_which == "body") {
	    $clause = "where body regexp '$qstring'";
	}
	elseif ($query_which == "both") {
	    $clause = "where title regexp '$qstring' ".
		"or body regexp '$qstring'";
	}
    }
    else {
	$wordarray = preg_split("/[\s,]+/", $query);

	foreach ($wordarray as $i => $word) {
	    if ($query_which == "title") {
		$wordarray[$i] = "title regexp '" . addslashes($word) . "'";
	    }
	    elseif ($query_which == "body") {
		$wordarray[$i] = "body regexp '" . addslashes($word) . "'";
	    }
	    else {
		$wordarray[$i] = "(title regexp '" . addslashes($word) . "' ".
		    "or body regexp '" . addslashes($word) . "')";
	    }
	}
	$clause = "where ". implode(" and ", $wordarray);
    }
    $search_result =
	DBQuery("select * from knowledge_base_entries ".
		"$clause ".
		($admin_access ? "" : 
		 "and section != 'Testbed Operations' ").
		"order by section,date_created");
}

if ( ! $search_result ) {
    if (!$embedded) {
	DBWarn("DB Query failed");
	SPITFORM($query, $query_type, $query_which,
                 "Bad query.  Please try again.  " . mysql_error());
	PAGEFOOTER();
    }
    return;
}
if (! mysql_num_rows($search_result)) {
    if (!$embedded) {
	SPITFORM($query, $query_type, $query_which,
		 "No Matches. Please try again");
	PAGEFOOTER();
    }
    return;
}

#
# Okay, format the list ...
#
if (!$embedded) {
    SPITFORM($query, $query_type, $query_which, null);
}

if (!$embedded) {
    echo "<blockquote><blockquote>\n";
}
echo "<font size=+2>Knowledge Base search results</font>\n";
echo "<ul>\n";

$lastsection = "";

while ($row = mysql_fetch_array($search_result)) {
    $section  = $row['section'];
    $title    = $row['title'];
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
	echo "<a href=kb-show.php3?xref_tag=$xref_tag>$title</a>\n";
    }
    else {
	echo "<a href=kb-show.php3?idx=$idx>$title</a>\n";
    }
}

echo "</ul></ul>\n";
if (!$embedded) {
    echo "</blockquote></blockquote>\n";
}

#
# Standard Testbed Footer
#
if (!$embedded) {
    PAGEFOOTER();
}
?>

