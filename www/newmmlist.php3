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

	if ($formfields[pid] == $project)
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
                         value=\"" . $formfields[listname] . "\"
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
                         value=\"" . $formfields[fullname] . "\"
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
                         size=8></td>
          </tr>\n";

    echo "<tr>
              <td colspan>*Retype Password:</td>
              <td class=left>
                  <input type=password
                         name=\"formfields[password2]\"
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
if (! $submit) {
    $defaults = array();

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
$errors = array();

#
# Project:
# 
if (!isset($formfields[pid]) ||
    strcmp($formfields[pid], "") == 0) {
    $errors["Project"] = "Not Selected";
}
elseif (!TBvalid_pid($formfields[pid])) {
    $errors["Project"] = "Invalid Characters";
}
elseif (!TBValidProject($formfields[pid])) {
    $errors["Project"] = "No such project";
}
elseif (!TBProjAccessCheck($uid, $formfields[pid],
			   $formfields[pid], $TB_PROJECT_READINFO)) {
    $errors["Project"] = "Not enough permission";
}
else {
    #
    # List Name, but only if pid was okay.
    #
    if (!isset($formfields[listname]) ||
	strcmp($formfields[listname], "") == 0) {
	$errors["List Name"] = "Missing Field";
    }
    else {
	$listname = $formfields[pid] . "-" . $formfields[listname];
	
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
	    $query_result =
		DBQueryFatal("select * from mailman_listnames ".
			     "where listname='$listname'");
	
	    if (mysql_num_rows($query_result)) {
		$errors["List Name"] = "Name already in use; pick another";
	    }
	}
    }
}

#
# Password
#
if (!isset($formfields[password1]) ||
    strcmp($formfields[password1], "") == 0) {
    $errors["Password"] = "Missing Field";
}
if (!isset($formfields[password2]) ||
    strcmp($formfields[password2], "") == 0) {
    $errors["Confirm Password"] = "Missing Field";
}
elseif (strcmp($formfields[password1], $formfields[password2])) {
    $errors["Confirm Password"] = "Does not match Password";
}
elseif (! TBvalid_userdata($formfields[password1])) {
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

$listname = $formfields[pid] . "-" . $formfields[listname];
$password = $formfields[password1];

#
# Need to lock the table for this. 
# 
DBQueryFatal("lock tables mailman_listnames write");

$query_result =
    DBQueryFatal("select * from mailman_listnames ".
		 "where listname='$listname'");
if (mysql_num_rows($query_result)) {
    DBQueryFatal("unlock tables");
    $errors["List Name"] = "Name already in use; pick another";
    SPITFORM($formfields, $errors);
    PAGEFOOTER();
    return;
}

DBQueryFatal("insert into mailman_listnames ".
	     " (listname, owner_uid, owner_idx, created) ".
	     "values ('$listname', '$uid', '$dbid', now())");
DBQueryFatal("unlock tables");

#
# Okay, call out to the backend to create the actual list. 
#
$retval = SUEXEC($uid, $TBADMINGROUP,
		 "webaddmmlist -u $listname $uid " . escapeshellarg($password),
		 SUEXEC_ACTION_IGNORE);

# Failed? Remove the DB entry.
if ($retval != 0) {
    DBQueryFatal("delete from mailman_listnames ".
		 "where listname='$listname'");
    SUEXECERROR(SUEXEC_ACTION_DIE);
}

#
# Okay, redirect the user over to the listadmin page to finish configuring.
#
header("Location: ${MAILMANURL}/admin/${listname}/?adminpw=${password}");
?>
