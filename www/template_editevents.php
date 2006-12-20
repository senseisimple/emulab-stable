<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("template_defs.php");

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Spit the form out using the array of data.
#
function SPITFORM($template, $formfields, $errors)
{
    PAGEHEADER("Edit Template Events");

    if ($template->EventList($eventlist) != 0) {
	TBERROR("Could not get eventlist for template!", 1);
    }

    $guid = $template->guid();
    $version = $template->vers();

    echo $template->PageHeader();
    echo "<br>\n";
    
    echo "<center>\n";
    $template->Show();
    echo "</center>\n";
    echo "<br>\n";

    if (!count($eventlist)) {
	echo "<center>";
	echo "<font size=+1 color=red>This template has no events!</font>\n";
	echo "</center>\n";
	return;
    }

    $guid = $template->guid();
    $version = $template->vers();

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

    echo "<form action=template_editevents.php?guid=$guid&version=$version ".
	 "      method=post>\n";
    echo "<table align=center border=1>\n";

    echo "<tr>";
    echo "<th align=center>Delete</th>\n";
    echo "<th align=center>Time</th>\n";
#    echo "<th align=center>Name</th>\n";
    echo "<th align=center>Node</th>\n";
    echo "<th align=center>Args</th>\n";
    echo "</tr><tr></tr>\n";

    #
    # Show a table of events with a delete button named by the event.
    #
    while (list ($index, $dbrow) = each ($eventlist)) {
	$vname       = $dbrow["vname"];
	$vnode       = $dbrow["vnode"];
	$delete_name = "delete_${vname}";
	$time_name   = "time_${vname}";
	$args_name   = "args_${vname}";
	    
	echo "<tr>";
	echo "<td align=center><input type=checkbox value=checked
                         name=\"formfields[$delete_name]\"
                         " . $formfields[$delete_name] . ">
              </td>\n";
	
	echo "<td class='pad4'>
                  <input type=text
                         name=\"formfields[$time_name]\"
                         value=\"" . $formfields[$time_name] . "\"
	                 size=10
                         maxlength=10>
              </td>\n";

#	echo "<td class='pad4'>$vname</td>\n";
	echo "<td class='pad4'>$vnode</td>\n";

	echo "<td class='pad4'>
                  <input type=text
                         name=\"formfields[$args_name]\"
                         value=\"" . $formfields[$args_name] . "\"
	                 size=64
                         maxlength=1024>
              </td>\n";

	echo "</tr>\n";
    }
    echo "<tr>
              <td class='pad4' align=center colspan=4>
                <b><input type=submit name=save value='Save Changes'></b>
              </td>
         </tr>
        </form>
        </table>\n";
}

#
# Verify page arguments.
# 
if (!isset($guid) ||
    strcmp($guid, "") == 0) {
    USERERROR("You must provide a template GUID.", 1);
}

if (!isset($version) ||
    strcmp($version, "") == 0) {
    USERERROR("You must provide a template version number", 1);
}
if (!TBvalid_guid($guid)) {
    PAGEARGERROR("Invalid characters in GUID!");
}
if (!TBvalid_integer($version)) {
    PAGEARGERROR("Invalid characters in version!");
}

#
# Check to make sure this is a valid template and enough permission.
#
$template = Template::Lookup($guid, $version);
if (!$template) {
    USERERROR("The experiment template $guid/$version is not a valid ".
              "experiment template!", 1);
}
if (! $template->AccessCheck($uid, $TB_EXPT_UPDATE)) {
    USERERROR("You do not have permission to instantiate experiment template ".
	      "$guid/$version!", 1);
}

# Event list master, used below.
if ($template->EventList($eventlist) != 0) {
    TBERROR("Could not get eventlist for template!", 1);
}

#
# On first load, display virgin form and exit.
#
if (!isset($save)) {
    $defaults = array();

    while (list ($index, $dbrow) = each ($eventlist)) {
	$vname       = $dbrow["vname"];
	$time        = $dbrow["time"];
	$arguments   = $dbrow["arguments"];
	$delete_name = "delete_${vname}";
	$time_name   = "time_${vname}";
	$args_name   = "args_${vname}";

	$defaults[$time_name] = $time;
	$defaults[$args_name] = $arguments;
    }
    
    SPITFORM($template, $defaults, 0);
    PAGEFOOTER();
    return;
}
elseif (! isset($formfields)) {
    PAGEARGERROR();
}

#
# Okay, validate form arguments.
#
$errors  = array();
$deletes = array();
$changes = array();

#
# Use the master event list from the template to validate.
#
while (list ($index, $dbrow) = each ($eventlist)) {
    $vname       = $dbrow["vname"];
    $time        = $dbrow["time"];
    $args        = $dbrow["args"];
    $delete_name = "delete_${vname}";
    $time_name   = "time_${vname}";
    $args_name   = "args_${vname}";

    # Delete button
    if (isset($formfields[$delete_name]) &&
	$formfields[$delete_name] == "checked") {
	
	$deletes[$vname] = $vname;
	continue;
    }
    
    # Modify the time.
    if (isset($formfields[$time_name]) && $formfields[$time_name] != "$time") {
	$newtime = $formfields[$time_name];
	
	if (!TBvalid_float($newtime)) {
	    $errors["$vname time"] = TBFieldErrorString();
	}
	elseif ($newtime < 0) {
	    $errors["$vname time"] = "Cannot be less then zero";
	}
	else {
	    if (!isset($changes[$vname])) {
		$changes[$vname] = array();
	    }
	    $changes[$vname]["time"] = $newtime;
	}
    }

    # Modify the arguments
    if (isset($formfields[$args_name]) && $formfields[$args_name] != "$args") {
	$newargs = $formfields[$args_name];

	if (! TBcheck_dbslot($newargs, 'eventlist','arguments',
			     TBDB_CHECKDBSLOT_WARN|TBDB_CHECKDBSLOT_ERROR)) {
	    $errors["$vname time"] = TBFieldErrorString();
	}
	else {
	    if (!isset($changes[$vname])) {
		$changes[$vname] = array();
	    }
	    $changes[$vname]["arguments"] = $newargs;
	}
    }
}

if (count($errors)) {
    SPITFORM($template, $formfields, $errors);
    PAGEFOOTER();
    exit(1);
}

#
# Do the deletes and changes.
#
reset($eventlist);

while (list ($index, $dbrow) = each ($eventlist)) {
    $vname       = $dbrow["vname"];

    if (isset($deletes[$vname])) {
	$template->DeleteEvent($vname);
	continue;
    }
    if (isset($changes[$vname])) {
	$template->ModifyEvent($vname, $changes[$vname]);
	continue;
    }
}

# Zap back to this page.
header("Location: template_editevents.php?guid=$guid&version=$version");
