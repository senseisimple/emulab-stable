<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# No Standard Testbed Header; going to spit out a redirect later.
#

# Some constants.
define("TBDB_KB_SECTIONLEN",	80);
define("TBDB_KB_TITLELEN",	140);
define("TBDB_KB_XREFTAGLEN",	30);
define("TBDB_KB_BODYLEN",	10000);

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

if (!$isadmin) {
    USERERROR("You are not allowed to view this page!", 1);
}

#
# Spit the form out using the array of data. 
# 
function SPITFORM($formfields, $errors)
{
    global	$idx, $action;
    
    PAGEHEADER("Add/Modify a Knowledge Base Entry");

    if (isset($idx)) {
	if ($action == "edit") {
	    echo "<center><h3>Modify Entry</h3>\n";
	}
	elseif ($action == "delete") {
	    echo "<center><h3>Delete Entry</h3>\n";
	}
	else {
	    echo "<center><h3>Add Entry</h3>\n";
	}
    }
    else {
	echo "<center><h3>Add Entry</h3>\n";
    }
    echo "(<a href=kb-search.php3>Search For Entries</a>)</center>\n";
    
    if ($errors) {
	echo "<table class=nogrid
                     align=center border=0 cellpadding=6 cellspacing=0>
              <tr>
                 <th align=center colspan=2>
                   <font size=+1 color=red>
                      &nbsp;Oops, please fix the following errors!&nbsp;
                   </font>
                 </td>
              </tr>\n";

	while (list ($name, $message) = each ($errors)) {
	    echo "<tr>
                     <td align=right>
                       <font color=red>$name:&nbsp;</font></td>
                     <td align=left>
                       <font color=red>$message</font></td>
                  </tr>\n";
	}
	echo "</table><br>\n";
    }

    echo "<br>
          <table align=center border=1> 
          <tr>
             <td align=center colspan=2>
                 <em>(Fields marked with * are required)</em>
             </td>
          </tr>
          <form action='kb-manage.php3' method=post>\n";

    $sections_result = 
	DBQueryFatal("select distinct section from knowledge_base_entries");

    #
    # If the form is being edited, then throw up some extra detail.
    #
    if (isset($idx)) {
	echo "<input type='hidden' name='idx' value='$idx'>\n";
	echo "<input type='hidden' name='action' value='$action'>\n";

	echo "<tr>
                  <td>Posted by:</td>
                  <td>$formfields[creator_uid]</td>
              </tr>\n";
	echo "<tr>
                  <td>Posted on:</td>
                  <td>$formfields[date_created]</td>
              </tr>\n";
	if (isset($formfields[date_modified])) {
	    echo "<tr>
                     <td>Last Modified by:</td>
                     <td>$formfields[modifier_uid]</td>
                  </tr>\n";
	    echo "<tr>
                      <td>Modified on:</td>
                      <td>$formfields[date_modified]</td>
                  </tr>\n";
	}
	echo "<tr></tr>\n";
    }

    #
    # Select Section. 
    #
    echo "<tr>
              <td>*Section Name:</td>
	      <td><select name=\"formfields[section]\">
		          <option value=none>Please Select </option>\n";

    while ($section_row = mysql_fetch_array($sections_result)) {
	$section  = $section_row['section'];
	$selected = "";

	if (isset($formfields[section]) &&
	    strcmp($formfields[section], $section) == 0)
	    $selected = "selected";

	echo "<option $selected value='$section'>$section &nbsp; </option>\n";
    }
    echo "       </select>\n";

    # And allow for a new section name to be created
    echo "    or add: ";
    echo "    <input type=text
                     name=\"formfields[new_section]\"
                     value=\"" . $formfields[new_section] . "\"
   	             size=50>\n";
    echo "   </td>
	  </tr>\n";

    #
    # Title
    #
    echo "<tr>
              <td>*Title:<br>
                  (a short pithy sentence)</td>
              <td class=left>
                  <input type=text
                         name=\"formfields[title]\"
                         value=\"" . $formfields[title] . "\"
	                 size=70>
              </td>
          </tr>\n";


    #
    # Make it a FAQ entry?
    #
    echo "<tr>
  	      <td>Faq Entry?</td>
               <td class=left>
                   <input type=checkbox
                          name=\"formfields[faq_entry]\"
                          value=1";

    if (isset($formfields[faq_entry]) && $formfields[faq_entry] == "1")
	echo "           checked";
	
    echo "                       > Yes
              </td>
          </tr>\n";
    
    #
    # Cross Reference Tag
    #
    echo "<tr>
              <td>Cross Reference Tag</td>
              <td class=left>
                  <input type=text
                         name=\"formfields[xref_tag]\"
                         value=\"" . $formfields[xref_tag] . "\"
	                 size=20>
              </td>
          </tr>\n";

    #
    # Body
    #
    echo "<tr>
              <td colspan=2>
               *Please enter (HTML) text of entry
              </td>
          </tr>
          <tr>
              <td colspan=2 align=center class=left>
                  <textarea name=\"formfields[body]\"
                    rows=30 cols=80>$formfields[body]</textarea>
              </td>
          </tr>\n";

    echo "<tr>
              <td align=center colspan=2>
                  <b><input type=submit name=submit ";
    if (isset($idx)) {
	if ($action == "edit") {
	    echo "value=Modify></b>";
	}
	elseif ($action == "delete") {
	    echo "value=Delete></b>";
	}
	else {
	    echo "value=Submit></b>";
	}
    }
    else {
	echo "value=Submit></b>";
    }
    echo "   </td>
          </tr>\n";

    echo "</form>
          </table>\n";

    echo "<br>
          <blockquote><blockquote>

          </blockquote></blockquote>\n";
}

#
# For delete/modify, check record.
# 
if (isset($idx)) {
    if (!isset($action) || !($action == "edit" || $action == "delete")) {
	PAGEARGERROR("Invalid action");
    }
    if (! TBvalid_integer($idx)) {
	PAGEARGERROR("Invalid characters in $idx");
    }
    $query_result =
	DBQueryFatal("select * from knowledge_base_entries ".
		     "where idx='$idx'");
    if (! mysql_num_rows($query_result)) {
	USERERROR("No such knowledge_base entry: $idx", 1);
    }
    $defaults = mysql_fetch_array($query_result);
    $defaults[body] = htmlspecialchars($defaults[body], ENT_QUOTES);
}

#
# On first load, display a virgin form and exit.
#
if (! $submit) {
    #
    # If doing an edit of an existing entry, then need to get that from
    # the DB. 
    #
    if (!isset($idx)) {
	$defaults = array();

        #
        # Allow formfields that are already set to override defaults
        #
	if (isset($formfields)) {
	    while (list ($field, $value) = each ($formfields)) {
		$defaults[$field] = $formfields[$field];
	    }
	}
    }
    
    SPITFORM($defaults, 0);
    PAGEFOOTER();
    return;
}

#
# Deal with deletion.
#
if (isset($submit) && $submit == "delete" && isset($idx)) {
    DBQueryFatal("delete from knowledge_base_entries where idx='$idx'");
    PAGEFOOTER();
    return;
}

#
# Otherwise, must validate and redisplay if errors
#
$errors = array();

#
# Section Name.
#
if (isset($formfields[new_section]) && $formfields[new_section] != "") {
    if (! TBvalid_userdata($formfields[new_section])) {
	$errors["Section"] = "Invalid characters";
    }
    elseif (strlen($formfields[new_section]) > TBDB_KB_SECTIONLEN) {
	$errors["Section"] =
	    "Too long! ".
	    "Must be less than or equal to " . TBDB_KB_SECTIONLEN;
    }
    $section = addslashes($formfields[new_section]);
}
elseif (isset($formfields[section]) &&
	$formfields[section] != "" && $formfields[section] != "none") {
    
    if (! TBvalid_userdata($formfields[section])) {
	$errors["Section"] = "Invalid characters";
    }
    elseif (strlen($formfields[section]) > TBDB_KB_SECTIONLEN) {
	$errors["Section"] =
	    "Too long! ".
	    "Must be less than or equal to " . TBDB_KB_SECTIONLEN;
    }
    $section = addslashes($formfields[section]);
}
else {
    $errors["Section"] = "Missing Field";
}

#
# Title
#
if (isset($formfields[title]) && $formfields[title] != "") {
    if (! TBvalid_userdata($formfields[title])) {
	$errors["Title"] = "Invalid characters";
    }
    elseif (strlen($formfields[title]) > TBDB_KB_TITLELEN) {
	$errors["Title"] =
	    "Too long! ".
	    "Must be less than or equal to " . TBDB_KB_TITLELEN;
    }
    $title = addslashes($formfields[title]);
}
else {
    $errors["Title"] = "Missing Field";
}

#
# Faq Entry?
#
if (isset($formfields[faq_entry]) && $formfields[faq_entry] == "1") {
    $faq_entry = 1;
}
else {
    $faq_entry = 0;
}

#
# Cross Reference Tag
#
if (isset($formfields[xref_tag]) && $formfields[xref_tag] != "") {
    if (! preg_match("/^[-\w]+$/", $formfields[xref_tag])) {
	$errors["Cross Reference Tag"] = "Invalid characters";
    }
    elseif (strlen($formfields[xref_tag]) > TBDB_KB_XREFTAGLEN) {
	$errors["Cross Reference Tag"] =
	    "Too long! ".
	    "Must be less than or equal to " . TBDB_KB_XREFTAGLEN;
    }
    $xref_tag = addslashes($formfields[xref_tag]);
    $xref_tag = "'$xref_tag'";
    
    #
    # Make sure the xref_tag is not already in use (by another entry).
    #
    $xref_result =
	DBQueryFatal("select idx from knowledge_base_entries ".
		     "where xref_tag=$xref_tag ".
		     (isset($idx) ? "and idx!='$idx'" : ""));
    
    if (mysql_num_rows($xref_result)) {
	$errors["Cross Reference Tag"] = "Already in use";
    }
}
else {
    $xref_tag = "NULL";
}

#
# Body
#
if (isset($formfields[body]) && $formfields[body] != "") {
    if (! TBvalid_fulltext($formfields[body])) {
	$errors["Body"] = "Invalid characters";
    }
    elseif (strlen($formfields[body]) > TBDB_KB_BODYLEN) {
	$errors["Body"] =
	    "Too long! ".
	    "Must be less than or equal to " . TBDB_KB_BODYLEN;
    }
    $body = addslashes($formfields[body]);
}
else {
    $errors["Body"] = "Missing Field";
}

#
# If any errors, respit the form with the current values and the
# error messages displayed. Iterate until happy.
# 
if (count($errors)) {
    SPITFORM($formfields, $errors);
    PAGEFOOTER();
    return;
}

#
# And insert the entry.
#
if (isset($idx)) {
    if ($action == "delete") {
	DBQueryFatal("delete from knowledge_base_entries ".
		     "where idx='$idx'");
    }
    else {
	DBQueryFatal("update knowledge_base_entries set ".
		     "  modifier_uid='$uid', ".
		     "  date_modified=now(), ".
		     "  section='$section', ".
		     "  title='$title', ".
		     "  body='$body', ".
		     "  faq_entry=$faq_entry, ".
		     "  xref_tag=$xref_tag ".
		     "where idx='$idx'");
    }
}
else {
    DBQueryFatal("insert into knowledge_base_entries set ".
		 "  idx=NULL, ".
		 "  creator_uid='$uid', ".
		 "  date_created=now(), ".
		 "  section='$section', ".
		 "  title='$title', ".
		 "  body='$body', ".
		 "  faq_entry=$faq_entry, ".
		 "  xref_tag=$xref_tag");

    $query_result = DBQueryFatal("select last_insert_id()");
    $row = mysql_fetch_array($query_result);
    $idx = $row[0];
}

if (isset($idx) && $action != "delete") {
    #
    # Okay, redirect the user over to the show page so they can see it.
    #
    header("Location: kb-show.php3?idx=$idx");
}
else {
    # Redirect to the search page.
    header("Location: kb-search.php3");
}
?>
