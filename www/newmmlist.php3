<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# No Standard Testbed Header; going to spit out a redirect later.
#

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$dbid      = $this_user->dbid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$optargs = OptionalPageArguments("submit",       PAGEARG_STRING,
				 "formfields",   PAGEARG_ARRAY);

#
# See what projects the uid can do this in.
#
$projlist = $this_user->ProjectAccessList($TB_PROJECT_READINFO);

if (! count($projlist)) {
    USERERROR("You do not appear to be a member of any Projects in which ".
	      "you have permission to create new mailing lists", 1);
}

#
# Spit the form out using the array of data. 
# 
function SPITFORM($formfields, $errors)
{
    global $TBDB_MMLENGTH, $projlist, $OURDOMAIN;

    PAGEHEADER("Create a new MailMan list");
    
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
    else {
	echo "<blockquote><blockquote>
              <center>
               <font size=+1>
                 Host your own project related mailing lists at $OURDOMAIN
               </font>
              </center><br>
              Use the form below to create a new mailing list. You will
              become the administrator for the new list, and are responsible
              for the list configuration and management, including user
              subscriptions, approval, etc. <b>Note that mailing lists
              should be related to your project in some manner; please,
              no lists discussing the latest episode of your favorite TV
              show.</b>
	      </blockquote></blockquote>\n";
    }

    echo "<SCRIPT LANGUAGE=JavaScript>
              function Changed(theform) 
              {
                  var pidx   = theform['formfields[pid]'].selectedIndex;
                  var pid    = theform['formfields[pid]'].options[pidx].value;
                  var list   = theform['formfields[listname]'].value;

                  if (pid == '') {
                      theform['formfields[fullname]'].value = '';
                  }
                  else if (list == '') {
                      theform['formfields[fullname]'].value = pid + '-';
                  }
                  else {
                      theform['formfields[fullname]'].value =
                        pid + '-' + theform['formfields[listname]'].value +
                        '@' + '$OURDOMAIN';
                  }
              }
          </SCRIPT>\n";

    echo "<br>
          <table align=center border=1> 
          <tr>
             <td align=center colspan=2>
                 <em>(Fields marked with * are required)</em>
             </td>
          </tr>
          <form action='newmmlist.php3' method=post name=myform>\n";

    #
    # Select Project
    #
    echo "<tr>
              <td>*Select Project:</td>
              <td><select name=\"formfields[pid]\"
                          onChange='Changed(myform);'>
                      <option value=''>Please Select &nbsp</option>\n";
    
    while (list($project) = each($projlist)) {
	$selected = "";

	if ($formfields["pid"] == $project)
	    $selected = "selected";
	
	echo "        <option $selected value='$project'>$project </option>\n";
    }
    echo "       </select>";
    echo "    </td>
          </tr>\n";

    #
    # Select List Name
    #
    echo "<tr>
              <td>*List Name (no blanks):</td>
              <td class=left>
                  <input type=text
                         onChange='Changed(myform);'
                         name=\"formfields[listname]\"
                         value=\"" . $formfields["listname"] . "\"
	                 size=$TBDB_MMLENGTH
                         maxlength=$TBDB_MMLENGTH>
              </td>
          </tr>\n";

    #
    # This is auto filled in by the javascript above.
    # 
    echo "<tr>
              <td>EMail Address (will be):</td>
              <td class=left>
                  <input type=text
                         readonly 
                         name=\"formfields[fullname]\"
                         value=\"" . $formfields["fullname"] . "\"
	                 size=$TBDB_MMLENGTH
                         maxlength=$TBDB_MMLENGTH>
              </td>
          </tr>\n";

    #
    # Password. Note that we do not resend the password. User
    # must retype on error.
    #
    echo "<tr>
              <td colspan>*Admin Password:</td>
              <td class=left>
                  <input type=password
                         name=\"formfields[password1]\"
                         value=\"" . $formfields["password1"] . "\"
                         size=8></td>
          </tr>\n";

    echo "<tr>
              <td colspan>*Retype Password:</td>
              <td class=left>
                  <input type=password
                         name=\"formfields[password2]\"
                         value=\"" . $formfields["password2"] . "\"
                         size=8></td>
         </tr>\n";

    echo "<tr>
              <td align=center colspan=2>
                  <b><input type=submit name=submit value=Submit></b>
              </td>
          </tr>\n";

    echo "</form>
          </table>\n";

    echo "<br>
          <blockquote><blockquote>
          After you click submit, the mailing list will be created on our
          server, and you will be automatically redirected to the list
          configuration page. Feel free to edit the configuration as you like.
          <br><br>
          Emulab mailing lists are maintained using the open source
          <a href=http://www.gnu.org/software/mailman/index.html>Mailman</a>
          package. You can find documentation for
          <a href=http://www.gnu.org/software/mailman/users.html>Users</a>
          and documentation for 
          <a href=http://www.gnu.org/software/mailman/admins.html>
          List Managers</a> on the Mailman
          <a href=http://www.gnu.org/software/mailman/docs.html>
          documentation</a> page.
          </blockquote></blockquote>\n";
}

#
# On first load, display a virgin form and exit.
#
if (!isset($submit)) {
    $defaults = array();
    $defaults["pid"]  = "";
    $defaults["password1"]   = "";
    $defaults["password2"]   = "";
    $defaults["listname"]    = "";
    $defaults["fullname"]    = "";

    #
    # Allow formfields that are already set to override defaults
    #
    if (isset($formfields)) {
	while (list ($field, $value) = each ($formfields)) {
	    $defaults[$field] = $formfields[$field];
	}
    }
    
    SPITFORM($defaults, 0);
    PAGEFOOTER();
    return;
}

#
# Otherwise, must validate and redisplay if errors
#
$errors  = array();
$project = null;

#
# Project:
#
if (!isset($formfields["pid"]) ||
    strcmp($formfields["pid"], "") == 0) {
    $errors["Project"] = "Not Selected";
}
elseif (!TBvalid_pid($formfields["pid"])) {
    $errors["Project"] = "Invalid project name";
}
elseif (! ($project = Project::Lookup($formfields["pid"]))) {
    $errors["Project"] = "Invalid project name";
}
elseif (! $project->AccessCheck($this_user, $TB_PROJECT_READINFO)) {
    $errors["Project"] = "Not enough permission";
}

#
# List Name, but only if pid was okay.
#
if ($project) {
    if (!isset($formfields["listname"]) ||
	strcmp($formfields["listname"], "") == 0) {
	$errors["List Name"] = "Missing Field";
    }
    else {
	$listname = $project->pid() . "-" . $formfields["listname"];
	
	if (! TBvalid_mailman_listname($listname)) {
	    $errors["List Name"] =
		"Must be alphanumeric and must begin with an alphanumeric";
	}
	elseif (strlen($listname) > $TBDB_MMLENGTH) {
	    $errors["List Name"] =
		"Too long! ".
		"Must be less than or equal to $TBDB_MMLENGTH";
	}
	else {
            #
            # Before we proceed, lets see if the list already exists.
            #
	    $safe_name = addslashes($listname);
	    
	    $query_result =
		DBQueryFatal("select * from mailman_listnames ".
			     "where listname='$safe_name'");
	
	    if (mysql_num_rows($query_result)) {
		$errors["List Name"] = "Name already in use; pick another";
	    }
	}
    }
}

#
# Password
#
if (!isset($formfields["password1"]) ||
    strcmp($formfields["password1"], "") == 0) {
    $errors["Password"] = "Missing Field";
}
if (!isset($formfields["password2"]) ||
    strcmp($formfields["password2"], "") == 0) {
    $errors["Confirm Password"] = "Missing Field";
}
elseif (strcmp($formfields["password1"], $formfields["password2"])) {
    $errors["Confirm Password"] = "Does not match Password";
}
elseif (! TBvalid_userdata($formfields["password1"])) {
    $errors["Password"] = "Invalid Characters";
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
# Build up argument array to pass along.
#
$args = array();

if (isset($formfields["pid"]) &&
    $formfields["pid"] != "none" && $formfields["pid"] != "") {
    $args["pid"]		= $formfields["pid"];
}
if (isset($formfields["password1"]) && $formfields["password1"] != "") {
    $args["password1"]	= $formfields["password1"];
}
if (isset($formfields["password2"]) && $formfields["password2"] != "") {
    $args["password2"]	= $formfields["password2"];
}
if (isset($formfields["listname"]) && $formfields["listname"] != "") {
    $args["listname"]	= $formfields["listname"];
}
if (isset($formfields["fullname"]) && $formfields["fullname"] != "") {
    $args["fullname"]	= $formfields["fullname"];
}

if (! ($result = NewMmList($uid, $args, $errors))) {
    # Always respit the form so that the form fields are not lost.
    # I just hate it when that happens so lets not be guilty of it ourselves.
    SPITFORM($formfields, $errors);
    PAGEFOOTER();
    return;
}

#
# Okay, redirect the user over to the listadmin page to finish configuring.
#
header("Location: ${MAILMANURL}/admin/${listname}/?adminpw=${formfields["password1"]}");

#
# When there's an MmList class, this will be a Class function to make a new one...
#
function NewMmList($uid, $args, &$errors) {
    global $suexec_output, $suexec_output_array, $TBADMINGROUP;

    #
    # Generate a temporary file and write in the XML goo.
    #
    $xmlname = tempnam("/tmp", "newmmlist");
    if (! $xmlname) {
	TBERROR("Could not create temporary filename", 0);
	$errors[] = "Transient error; please try again later.";
	return null;
    }
    if (! ($fp = fopen($xmlname, "w"))) {
	TBERROR("Could not open temp file $xmlname", 0);
	$errors[] = "Transient error; please try again later.";
	return null;
    }

    fwrite($fp, "<MmList>\n");
    foreach ($args as $name => $value) {
	fwrite($fp, "<attribute name=\"$name\">");
	fwrite($fp, "  <value>" . htmlspecialchars($value) . "</value>");
	fwrite($fp, "</attribute>\n");
    }
    fwrite($fp, "</MmList>\n");
    fclose($fp);
    chmod($xmlname, 0666);

    $retval = SUEXEC($uid, $TBADMINGROUP, "webnewmmlist $xmlname",
		     SUEXEC_ACTION_IGNORE);

    if ($retval) {
	if ($retval < 0) {
	    $errors[] = "Transient error; please try again later.";
	    SUEXECERROR(SUEXEC_ACTION_CONTINUE);
	}
	else {
	    # unlink($xmlname);
	    if (count($suexec_output_array)) {
		for ($i = 0; $i < count($suexec_output_array); $i++) {
		    $line = $suexec_output_array[$i];
		    if (preg_match("/^([-\w]+):\s*(.*)$/",
				   $line, $matches)) {
			$errors[$matches[1]] = $matches[2];
		    }
		    else
			$errors[] = $line;
		}
	    }
	    else
		$errors[] = "Transient error; please try again later.";
	}
	return null;
    }

    # There are no return value(s) to parse at the end of the output.

    # Unlink this here, so that the file is left behind in case of error.
    # We can then create the MmList by hand from the xmlfile, if desired.
    unlink($xmlname);

    return true; 
}

?>
