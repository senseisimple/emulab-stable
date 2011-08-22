<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2011 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$optargs = OptionalPageArguments("project",    PAGEARG_PROJECT,
				 "submit",     PAGEARG_STRING,
				 "formfields", PAGEARG_ARRAY);
if (!isset($project)) {
    #
    # See what projects the uid can do this in.
    #
    $projlist = $this_user->ProjectAccessList($TB_PROJECT_MAKEGROUP);

    if (! count($projlist)) {
	USERERROR("You do not appear to be a member of any Projects in which ".
		  "you have permission to create new groups.", 1);
    }
}
else {
    #
    # Verify permission for specific project.
    #
    $pid = $project->pid();
    
    if (!$project->AccessCheck($this_user, $TB_PROJECT_MAKEGROUP)) {
	USERERROR("You do not have permission to create groups in ".
		  "project $pid!", 1);
    }
}

#
# Standard Testbed Header
#
PAGEHEADER("Create a Project Group");

#
# Spit the form out using the array of data. 
# 
function SPITFORM($formfields, $errors)
{
    global $project, $pid, $projlist;
    global $TBDB_GIDLEN, $TBDB_UIDLEN;
    global $WIKIDOCURL;

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
          </tr>\n";

    if (isset($project)) {
	$url = CreateURL("newgroup", $project);
	echo "<form action='$url' method=post>
	      <tr>
		  <td>* Project:</td>
		  <td class=left>
		      <input name=project type=readonly value='$pid'>
		  </td>
	      </tr>\n";
    }
    else {
	$url = CreateURL("newgroup");
	echo "<form action='$url' method=post>
	      <tr>
		  <td>*Select Project:</td>";
	echo "    <td><select name=project>";

	while (list($proj) = each($projlist)) {
	    echo "<option value='$proj'>$proj </option>\n";
	}

	echo "       </select>";
	echo "    </td>
	      </tr>\n";
    }

    echo "<tr>
	      <td>*Group Name (no blanks, lowercase):</td>
	      <td class=left>
		  <input type=text 
			 name=\"formfields[group_id]\" 
			 value=\"" . $formfields["group_id"] . "\"
			 size=$TBDB_GIDLEN
			 maxlength=$TBDB_GIDLEN>
	      </td>
	  </tr>\n";

    echo "<tr>
	      <td>*Group Description:</td>
	      <td class=left>
		  <input type=text size=50
			 name=\"formfields[group_description]\"
			 value=\"" . $formfields["group_description"] . "\">
	      </td>
	  </tr>\n";

    echo "<tr>
	      <td>*Group Leader (Emulab userid):</td>
	      <td class=left>
		  <input type=text
			 name=\"formfields[group_leader]\"
			 value=\"" . $formfields["group_leader"] . "\"
			 size=$TBDB_UIDLEN maxlength=$TBDB_UIDLEN>
	      </td>
	  </tr>\n";

    echo "<tr>
              <td align=center colspan=2>
                  <b><input type=submit name=submit value=Submit></b>
              </td>
          </tr>\n";

    echo "</form>
          </table>\n";

    echo "<br><center>
	     Important <a href='$WIKIDOCURL/Groups#SECURITY'>
	     security issues</a> are discussed in the
	     <a href='$WIKIDOCURL/Groups'>Groups Tutorial</a>.
          </center>\n";
}

#
# Accumulate error reports for the user, e.g.
#    $errors["Key"] = "Msg";
# Required page args may need to be checked early.
$errors  = array();

#
# On first load, display a virgin form and exit.
#
if (!isset($submit)) {
    $defaults = array();
    $defaults["group_id"]	   = "";
    $defaults["group_description"] = "";
    $defaults["group_leader"]	   = $uid;

    SPITFORM($defaults, $errors);
    PAGEFOOTER();
    return;
}

#
# Build up argument array to pass along.
#
$args = array();

if (isset($formfields["project"]) &&
    $formfields["project"] != "none" && $formfields["project"] != "") {
    $args["project"]		= $formfields["project"];
}
if (isset($formfields["group_id"]) && $formfields["group_id"] != "") {
    $args["group_id"]	= $formfields["group_id"];
}
if (isset($formfields["group_description"]) && 
    $formfields["group_description"] != "") {
    $args["group_description"]	= $formfields["group_description"];
}
if (isset($formfields["group_leader"]) && $formfields["group_leader"] != "") {
    $args["group_leader"]	= $formfields["group_leader"];
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

$group_id = $formfields["group_id"];
###STARTBUSY("Creating project group $group_id.");
echo "<br>Creating project group $group_id.<br>\n";
flush();

if (! ($newgroup = Group::Create($project, $uid, $args, $errors))) {
    # Always respit the form so that the form fields are not lost.
    # I just hate it when that happens so lets not be guilty of it ourselves.
    SPITFORM($formfields, $errors);
    PAGEFOOTER();
    return;
}

###STOPBUSY();

echo "<center><h3>Done!</h3></center>\n";
PAGEREPLACE(CreateURL("showgroup", $newgroup));

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
