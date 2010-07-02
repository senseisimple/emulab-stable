<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("template_defs.php");

#
# No PAGEHEADER since we spit out a Location header later. See below.
#

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments
#
$reqargs = RequiredPageArguments("action",        PAGEARG_STRING);
$optargs = OptionalPageArguments("template",      PAGEARG_TEMPLATE,
				 "submit",        PAGEARG_STRING,
				 "metadata",      PAGEARG_METADATA,
				 "metadata_type", PAGEARG_STRING,
				 "referrer",      PAGEARG_STRING,
				 "formfields",    PAGEARG_ARRAY);

# Need these below.
$guid = $template->guid();
$vers = $template->vers();
$pid  = $template->pid();
$unix_gid = $template->UnixGID();

#
# Spit the form out using the array of data.
#
function SPITFORM($action, $formfields, $errors)
{
    global $template, $metadata, $referrer;
    global $metadata_type;

    $template_guid = $template->guid();
    $template_vers = $template->vers();
    
    PAGEHEADER("Manage Template Metadata");

    if ($action == "add") {
	echo "<center>";
	echo "<font size=+1>
                  Attach metadata[<b>1</b>] to your template.</font>";
	echo "</center><br>\n";
    }
    elseif ($action == "delete") {
	echo "<center>";
	echo "<h3>Are you sure you want to delete this item?</h3>";
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

    if ($action == "modify" || $action == "delete") {
	$url = CreateURL("template_metadata", $template, $metadata);
    }
    else {
	$url = CreateURL("template_metadata", $template);
    }
    if (isset($metadata_type) && $metadata_type != "") {
	$url .= "&metadata_type=$metadata_type";
    }
    
    echo "<form action='${url}&action=$action' method=post>\n";
    echo "<table align=center border=1>\n";

    if (isset($referrer) && $referrer != "") {
	echo "<input type=hidden name=referrer value='$referrer'>";
    }

    #
    # Template GUID and Version. These are read-only fields.
    #
    echo "<tr>
              <td class='pad4'>Template GUID:</td>
              <td class='pad4' class=left>
                  $template_guid/$template_vers</td>\n";
    echo "</tr>\n";

    if ($action == "modify" || $action == "delete") {
	$metadata_guid = $metadata->guid();
	$metadata_vers = $metadata->vers();
	
	echo "<tr>
                  <td class='pad4'>Metadata GUID:</td>
                  <td class='pad4' class=left>
                      $metadata_guid/$metadata_vers</td>\n";
	echo "</tr>\n";
    }

    $readonly_name  = ($action == "add"    ? "" : "readonly");
    $readonly_value = ($action == "delete" ? "readonly" : "");

    #
    # Name of the item
    #
    echo "<tr>
              <td>*Name:<br>
                  (something short and pithy)</td>
              <td class=pad4 class=left>
	          <input type=text $readonly_name
                         name=\"formfields[name]\"
                         value=\"" . $formfields["name"] . "\"
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
                  <textarea $readonly_value name=\"formfields[value]\"
                    rows=10 cols=80>" .
	            ereg_replace("\r", "", $formfields["value"]) .
	           "</textarea>
              </td>
          </tr>\n";

    if ($action == "modify") {
	$tag = "Modify Metadata";
    }
    elseif ($action == "delete") {
	$tag = "Delete Metadata";
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

    echo "<blockquote><blockquote>
          <ol>
            <li> Metadata can be any arbitrary name/value pair that you want
                 to associate with your template. The name can include
                 any printable ascii character including spaces, but
                 not newlines. The value can include any printable ascii
                 character and my be multiline.
          </ol>
          </blockquote></blockquote>\n";
}

#
# On first load, display virgin form and exit.
#
if (!isset($submit)) {
    #
    # In show mode, we can show any metadata entry, but it cannot be modified
    # unless its in the context of a template. That might change later?
    #
    if ($action == "show") {
	if (!isset($metadata)) {
	    PAGEARGERROR("Must provide a metadata guid");
	}
	$metadata_guid = $metadata->guid();
	$metadata_vers = $metadata->vers();

        #
        # Verify Permission. Need permission for the template, any version.
        #
	if (! isset($template)) {
	    $template = Template::Lookup($metadata->template_guid(), 1);
	}

	if (!$template ||
	    !$template->AccessCheck($this_user, $TB_EXPT_READINFO)) {
	    USERERROR("You do not have permission to view metadata in ".
		      " template $template_guid!", 1);
	}

	PAGEHEADER("Show Metadata");
	$metadata->Show();
	PAGEFOOTER();
	return;
    }
    elseif ($action == "modify" || $action == "delete") {
	if (!isset($template)) {
	    PAGEARGERROR("Must provide a template guid");
	}
	$template_guid = $template->guid();
	$template_vers = $template->vers();

	if (!isset($metadata)) {
	    PAGEARGERROR("Must provide a metadata guid");
	}
	$metadata_guid = $metadata->guid();
	$metadata_vers = $metadata->vers();
	$metadata_type = $metadata->type();
    }
    else {
	if (!isset($template)) {
	    PAGEARGERROR("Must provide a template guid");
	}
	$template_guid = $template->guid();
	$template_vers = $template->vers();

	if (isset($metadata_type) && $metadata_type != "") {
	    if (!TBvalid_template_metadata_type($metadata_type)) {
		PAGEARGERROR("Invalid characters in metadata type!");
	    }
	}
	else {
	    unset($metadata_type);
	}
    }

    # Perm check for add/modify to the template.
    if (!$template->AccessCheck($this_user, $TB_EXPT_MODIFY)) {
	USERERROR("You do not have permission to $action metadata in ".
		  " template $template_guid!", 1);
    }

    # Defaults for the form come from the DB.
    $defaults = array();
    if ($action == "modify" || $action == "delete") {
	$defaults["name"]  = $metadata->name();
	$defaults["value"] = $metadata->value();
    }
    else {
	$defaults["name"]  = "";
	$defaults["value"] = "";
    }
    $referrer = $_SERVER['HTTP_REFERER'];
    
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
if ($action == "modify" || $action == "delete") {
    if (!isset($template)) {
	PAGEARGERROR("Must provide a template guid");
    }
    $template_guid = $template->guid();
    $template_vers = $template->vers();

    if (!isset($metadata)) {
	PAGEARGERROR("Must provide a metadata guid");
    }
    $metadata_guid = $metadata->guid();
    $metadata_vers = $metadata->vers();
    $metadata_type = $metadata->type();
}
else {
    if (!isset($template)) {
	PAGEARGERROR("Must provide a template guid");
    }
    $template_guid = $template->guid();
    $template_vers = $template->vers();

    if (isset($metadata_type) && $metadata_type != "") {
	if (!TBvalid_template_metadata_type($metadata_type)) {
	    PAGEARGERROR("Invalid characters in metadata type!");
	}
    }
    else {
	unset($metadata_type);
    }
}

# Perm check for add/modify to the template.
if (!$template->AccessCheck($this_user, $TB_EXPT_MODIFY)) {
    USERERROR("You do not have permission to $action metadata in ".
	      " template $template_guid!", 1);
}

#
# Okay, validate form arguments.
#
$errors = array();
$command_opts = "";

#
# Name
#
if (!isset($formfields["name"]) || $formfields["name"] == "") {
    $errors["Metadata Name"] = "Missing Field";
}
elseif (!TBvalid_template_metadata_name($formfields["name"])) {
    $errors["Metadata Name"] = TBFieldErrorString();
}

if ($action == "add") {
    if ($template->LookupMetadataByName($formfields["name"])) {
	$errors["Metadata Name"] = "Name already in use";
    }
    if (isset($metadata_type)) {
	$command_opts .= "-t " . escapeshellarg($metadata_type) . " ";
    }
    $command_opts .= "-a add " . escapeshellarg($formfields["name"]);
}
elseif ($action == "delete") {
    $command_opts .= "-a delete " . escapeshellarg($formfields["name"]);
}
else {
    # Had to already exist above. 
    $command_opts .= "-a modify " . escapeshellarg($formfields["name"]);
}

#
# Value:
#
if ($action != "delete" && $action != "add") {
    if (!isset($formfields["value"]) || $formfields["value"] == "") {
	$errors["Metadata Value"] = "Missing Field";
    }
    elseif (!TBvalid_template_metadata_value($formfields["value"])) {
	$errors["Metadata Value"] = TBFieldErrorString();
    }
    if ($action == "modify" &&
	$formfields["value"] == $metadata->value()) {
	$errors["Metadata Value"] = "New value identical to old value";
    }
}

#
# XXX Some metadata is special ...
#
if (isset($metadata_type)) {
    if ($metadata_type == "tid") {
	if ($action == "delete") {
	    $errors["TID"] = "Not allowed to delete this";
	}
	elseif (!TBvalid_eid($formfields["value"])) {
	    $errors["TID"] = TBFieldErrorString();
	}
    }
    elseif ($metadata_type == "template_description") {
	if ($action == "delete") {
	    $errors["Description"] = "Not allowed to delete this";
	}
	elseif (!TBvalid_template_description($formfields["value"])) {
	    $errors["Description"] = TBFieldErrorString();
	}
    }
    elseif ($metadata_type == "parameter_description") {
	if (!TBvalid_template_parameter_description($formfields["value"])) {
	    $errors["Description"] = TBFieldErrorString();
	}
    }
    elseif ($metadata_type == "instance_description") {
	if (!TBvalid_template_instance_description($formfields["value"])) {
	    $errors["Description"] = TBFieldErrorString();
	}
    }
    elseif ($metadata_type == "run_description") {
	if (!TBvalid_experiment_run_description($formfields["value"])) {
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
# Generate a temporary file and write in the data.
#
if ($action != "delete") {
    list($usec, $sec) = explode(' ', microtime());
    srand((float) $sec + ((float) $usec * 100000));
    $foo = rand();

    $datafile = "/tmp/$uid-$foo.txt";

    if (! ($fp = fopen($datafile, "w"))) {
	TBERROR("Could not create temporary file $datafile", 1);
    }

    fwrite($fp, $formfields["value"]);
    fclose($fp);
    chmod($datafile, 0666);

    $command_opts = " -f $datafile $command_opts";
}

#
# The backend does the actual work.
#
$pid = $template->pid();
$gid = $template->gid();

$retval = SUEXEC($uid, "$pid,$unix_gid",
		 "webtemplate_metadata ".
		 "$command_opts $template_guid/$template_vers",
		 SUEXEC_ACTION_IGNORE);

if ($action != "delete") {
    unlink($datafile);
}

#
# Fatal Error. Report to the user, even though there is not much he can
# do with the error. Also reports to tbops.
# 
if ($retval < 0) {
    SUEXECERROR(SUEXEC_ACTION_CONTINUE);
}

# User error. Tell user and exit.
if ($retval) {
    SUEXECERROR(SUEXEC_ACTION_USERERROR);
    return;
}

if (isset($referrer)) {
    header("Location: $referrer");
}
else {
    header("Location: ". CreateURL("template_show", $template));
}

?>
