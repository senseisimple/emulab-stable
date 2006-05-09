<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("template_defs.php");

#
# No PAGEHEADER since we spit out a Location header later. See below.
#

#
# Only known and logged in users.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

#
# Spit the form out using the array of data.
#
function SPITFORM($action, $formfields, $errors)
{
    global $template_guid, $template_vers;
    global $metadata_guid, $metadata_vers;
    
    PAGEHEADER("Manage Template Metadata");

    if ($action == "add") {
	echo "<center>";
	echo "<h3>Use this page to attach metadata to your template.</h3>";
	echo "</center><br>\n";
    }

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

    echo "<form action=template_metadata.php?action=$action method=post>\n";
    echo "<table align=center border=1>\n";

    #
    # Template GUID and Version. These are read-only fields.
    #
    echo "<tr>
              <td class='pad4'>Template GUID:</td>
              <td class='pad4' class=left>
                  $template_guid/$template_vers</td>\n";
    echo "</tr>\n";
    echo "<input type=hidden name=template_guid value=$template_guid>\n";
    echo "<input type=hidden name=template_vers value=$template_vers>\n";

    if ($action == "modify") {
	echo "<tr>
                  <td class='pad4'>Metadata GUID:</td>
                  <td class='pad4' class=left>
                      $metadata_guid/$metadata_vers</td>\n";
	echo "</tr>\n";
	echo "<input type=hidden name=metadata_guid value=$metadata_guid>\n";
	echo "<input type=hidden name=metadata_vers
                                 value=$metadata_vers>\n";
    }

    $readonly = ($action == "modify" ? "readonly" : "");

    #
    # Name of the item
    #
    echo "<tr>
              <td>*Name:<br>
                  (something short and pithy)</td>
              <td class=pad4 class=left>
	          <input type=text $readonly
                         name=\"formfields[name]\"
                         value=\"" . $formfields[name] . "\"
	                 size=64>
             </td>
          </tr>\n";

    echo "<tr>
              <td colspan=2>
               Value (use this area to enter the value of your metadata item).
              </td>
          </tr>
          <tr>
              <td colspan=2 align=center class=left>
                  <textarea name=\"formfields[value]\"
                    rows=10 cols=80>" .
	            ereg_replace("\r", "", $formfields[value]) .
	           "</textarea>
              </td>
          </tr>\n";

    if ($action == "modify") {
	$tag = "Modify Metadata";
    }
    else {
	$tag = "Add Metadata";
    }    
 
    echo "<tr>
              <td class='pad4' align=center colspan=2>
                 <b><input type=submit name=submit value='$tag'></b>
              </td>
         </tr>
        </form>
        </table>\n";
}

#
# On first load, display virgin form and exit.
#
if (!isset($submit)) {
    #
    # Verify page arguments.
    # 
    if (!isset($guid) ||
	strcmp($guid, "") == 0) {
	USERERROR("You must provide a GUID.", 1);
    }
    if (!isset($version) ||
	strcmp($version, "") == 0) {
	USERERROR("You must provide a version", 1);
    }
    if (!TBvalid_guid($guid)) {
	PAGEARGERROR("Invalid characters in GUID!");
    }
    if (!TBvalid_integer($version)) {
	PAGEARGERROR("Invalid characters in version!");
    }

    #
    # In show mode, we can show any metadata entry, but it cannot be modified
    # unless its in the context of a template. That might change later?
    #
    if ($action == "show") {
	$metadata_guid = $guid;
	$metadata_vers = $version;

	#
	# Find template associated with this metadata item (for perm check).
	#
	if (! TBMetadataTemplate($guid, $version, $template_guid)) {
	    USERERROR("Invalid metadata $guid/$version", 1);
	}
    }
    elseif ($action == "modify") {
	$template_guid = $guid;
	$template_vers = $version;

	# Must get the metadata guid and vers we want to change.
	if (!isset($metadata_guid) || $metadata_guid == "") {
	    USERERROR("You must provide a metadata GUID", 1);
	}
	if (!isset($metadata_vers) || $metadata_vers == "") {
	    USERERROR("You must provide a metadata version", 1);
	}
	if (!TBvalid_guid($metadata_guid)) {
	    PAGEARGERROR("Invalid characters in GUID!");
	}
	if (!TBvalid_integer($metadata_vers)) {
	    PAGEARGERROR("Invalid characters in metadata version!");
	}

	#
	# Verify this metadata is attached to the template.
	#
	if (! TBValidTemplateMetadata($metadata_guid, $metadata_vers,
				      $template_guid, $template_vers)) {
	    USERERROR("Invalid metadata $guid/$version", 1);
	}
    }
    else {
	$template_guid = $guid;
	$template_vers = $version;

        #
        # Check to make sure this is a valid template.
        #
	if (! TBValidExperimentTemplate($guid, $version)) {
	    USERERROR("The experiment template $guid/$version is not a valid ".
		      "experiment template!", 1);
	}
    }
    
    #
    # Verify Permission.
    #
    if (! TBExptTemplateAccessCheck($uid, $template_guid, $TB_EXPT_MODIFY)) {
	USERERROR("You do not have permission to $action metadata in ".
		  " template $template_guid!", 1);
    }

    #
    # For plain show ...
    #
    if ($action == "show") {
	PAGEHEADER("Show Metadata");
	SHOWMETADATAITEM($guid, $version);	
	PAGEFOOTER();
	return;
    }

    if ($action == "modify") {
	if (! TBMetadataData($metadata_guid, $metadata_vers, $defaults)) {
	    USERERROR("Could not retrieve metadata data", 1);
	}
    }
    else {
	$defaults = array();
    }
    
    #
    # Allow formfields that are already set to override defaults
    #
    if (isset($formfields)) {
	while (list ($field, $value) = each ($formfields)) {
	    $defaults[$field] = $formfields[$field];
	}
    }

    SPITFORM($action, $defaults, 0);
    PAGEFOOTER();
    return;
}
elseif (! isset($formfields)) {
    PAGEARGERROR();
}

#
# Verify page arguments, which depend on action.
#
if ($action == "modify") {
    if (!isset($metadata_guid) || $metadata_guid == "") {
	USERERROR("You must provide a Metadata GUID.", 1);
    }
    if (!isset($metadata_vers) || $metadata_vers == "") {
	USERERROR("You must provide a Metadata version", 1);
    }
    if (!TBvalid_guid($metadata_guid)) {
	PAGEARGERROR("Invalid characters in GUID!");
    }
    if (!TBvalid_integer($metadata_vers)) {
	PAGEARGERROR("Invalid characters in version!");
    }
    if (!isset($template_guid) || $template_guid == "") {
	USERERROR("You must provide a Template GUID.", 1);
    }
    if (!isset($template_vers) || $template_vers == "") {
	USERERROR("You must provide a Template version", 1);
    }
    if (!TBvalid_guid($template_guid)) {
	PAGEARGERROR("Invalid characters in GUID!");
    }
    if (!TBvalid_integer($template_vers)) {
	PAGEARGERROR("Invalid characters in version!");
    }
    
    #
    # Verify this metadata is attached to the template.
    #
    if (! TBValidTemplateMetadata($metadata_guid, $metadata_vers,
				  $template_guid, $template_vers)) {
	USERERROR("Invalid metadata $guid/$version", 1);
    }

    # Need this below.
    if (! TBMetadataData($metadata_guid, $metadata_vers, $metadata_data)) {
	USERERROR("Could not retrieve metadata data", 1);
    }
}
else {
    if (!isset($template_guid) || $template_guid == "") {
	USERERROR("You must provide a Template GUID.", 1);
    }
    if (!isset($template_vers) || $template_vers == "") {
	USERERROR("You must provide a Template version", 1);
    }
    if (!TBvalid_guid($template_guid)) {
	PAGEARGERROR("Invalid characters in GUID!");
    }
    if (!TBvalid_integer($template_vers)) {
	PAGEARGERROR("Invalid characters in version!");
    }

    #
    # Check to make sure this is a valid template.
    #
    if (! TBValidExperimentTemplate($template_guid, $template_vers)) {
	USERERROR("The experiment template $template_guid/$template_vers ".
		  "is not a valid experiment template!", 1);
    }
}

#
# Verify Permission.
#
if (! TBExptTemplateAccessCheck($uid, $template_guid, $TB_EXPT_MODIFY)) {
    USERERROR("You do not have permission to modify experiment template ".
	      "$template_guid/$template_vers!", 1);
}

#
# Okay, validate form arguments.
#
$errors = array();

#
# Name
#
if ($action == "add") {
    if (!isset($formfields[name]) || $formfields[name] == "") {
	$errors["Metadata Name"] = "Missing Field";
    }
    elseif (!TBvalid_template_metadata_name($formfields[name])) {
	$errors["Metadata Name"] = TBFieldErrorString();
    }
    elseif (TBTemplateMetadataLookup($template_guid, $template_vers,
				     $formfields[name], $metadata_value)) {
	$errors["Metadata Name"] = "Name already in use"; 
    }
}

#
# Value:
# 
if (!isset($formfields[value]) || $formfields[value] == "") {
    $errors["Metadata Value"] = "Missing Field";
}
elseif (!TBvalid_template_metadata_value($formfields[value])) {
    $errors["Metadata Value"] = TBFieldErrorString();
}
if ($action == "modify" &&
    $formfields[value] == $metadata_data[value]) {
    $errors["Metadata Value"] = "New value identical to old value";
}

#
# XXX Some metadata is special ...
#
if ($action == "modify") {
    if ($formfields[name] == "TID") {
	if (!TBvalid_eid($formfields[value])) {
	    $errors["TID"] = TBFieldErrorString();
	}
    }
    elseif ($formfields[name] == "description") {
	if (!TBvalid_template_description($formfields[value])) {
	    $errors["Description"] = TBFieldErrorString();
	}
    }
}

if (count($errors)) {
    SPITFORM($action, $formfields, $errors);
    PAGEFOOTER();
    exit(1);
}

#
# Insert the new record.
#
$name  = addslashes($formfields[name]);
$value = addslashes($formfields[value]);

if ($action == "modify") {
    $query_result =
	DBQueryFatal("select MAX(vers) ".
		     " from experiment_template_metadata_items as maxvers ".
		     "where guid='$metadata_guid'");

    $row  = mysql_fetch_array($query_result);
    $maxvers = $row[0];
    $extrastuff = "parent_guid='$metadata_guid',parent_vers='$metadata_vers',";
    $metadata_newvers = $maxvers + 1;
}
else {
    TBNewGUID($metadata_guid);
    $metadata_newvers = 1;
    $extrastuff = "";
}

#
# Insert new item and get back the index.
#
DBQueryFatal("insert into experiment_template_metadata_items set ".
	     "     guid='$metadata_guid',vers='$metadata_newvers', ".
	     "     template_guid='$template_guid', uid='$uid', ".
	     $extrastuff .
	     "     name='$name', value='$value', created=now()");

#
# Now we can insert (or update) the record for the template.
#
if ($action == "modify") {
    DBQueryFatal("update experiment_template_metadata ".
		 "  set metadata_vers='$metadata_newvers' ".
		 "where metadata_guid='$metadata_guid' and ".
		 "      metadata_vers='$metadata_vers'");		 
}
else {
    DBQueryFatal("insert into experiment_template_metadata set ".
		 "   metadata_guid='$metadata_guid', ".
		 "   metadata_vers='$metadata_newvers', ".
		 "   parent_guid='$template_guid', ".
		 "   parent_vers='$template_vers'");
}

#
# XXX Some metadata is special ...
#
if ($action == "modify") {
    if ($name == "TID") {
	DBQueryFatal("update experiment_templates set tid='$value' ".
		     "where guid='$template_guid' and ".
		     "      vers='$template_vers'");
    }
    elseif ($name == "description") {
	DBQueryFatal("update experiment_templates set description='$value' ".
		     "where guid='$template_guid' and ".
		     "      vers='$template_vers'");
    }
}

if (isset($template_guid) && isset($template_vers)) {
    header("Location: ".
	   "template_show.php?guid=$template_guid&version=$template_vers");
}
else {
    #
    # Zap back to this page, but with show option.
    #
    header("Location: ".
	   "template_metadata.php?action=show&guid=$metadata_guid".
	   "&version=$metadata_newvers");
}

