<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Commit and Tag");

#
# Only known and logged in users can look at experiments.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$reqargs = RequiredPageArguments("experiment", PAGEARG_EXPERIMENT);
$optargs = OptionalPageArguments("submit", PAGEARG_STRING,
				 "formfields", PAGEARG_ARRAY);

# Need these below.
$pid = $experiment->pid();
$eid = $experiment->eid();
$gid = $experiment->gid();

# Permission
if (!$isadmin &&
    !$experiment->AccessCheck($this_user, $TB_PROJECT_READINFO)) {
    USERERROR("You do not have permission to view tags for ".
	      "archive in $pid/$eid!", 1);
}

function SPITFORM($formfields, $errors)
{
    global $experiment, $TBDB_ARCHIVE_TAGLEN, $referrer;

    echo "<center>
          Commit/Tag Archive
          </center><br>\n";

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

    echo "<table align=center border=1> 
          <form action='" . CreateURL("archive_tag", $experiment) . "' ".
	        "method=post>\n";

    if (isset($referrer)) {
	echo "<input type=hidden name=referrer value=$referrer>\n";
    }
    
    echo "<tr>
              <td align=center>
               <b>Please enter a tag[<b>1</b>]</b>
              </td>
          </tr>\n";

    echo "<tr>
              <td class=left>
                  <input type=text
                         name=\"formfields[tag]\"
                         value=\"" . $formfields["tag"] . "\"
	                 size=$TBDB_ARCHIVE_TAGLEN
                         maxlength=$TBDB_ARCHIVE_TAGLEN>
          </tr>\n";


    echo "<tr>
              <td align=center>
               <b>Please enter an optional message to be logged
                       with the tag.</b>
              </td>
          </tr>
          <tr>
              <td align=center class=left>
                  <textarea name=\"formfields[message]\"
                    rows=10 cols=70>" .
	            ereg_replace("\r", "", $formfields["message"]) .
	            "</textarea>
              </td>
          </tr>\n";

    echo "<tr>
              <td align=center>
                 <b><input type=submit name=submit value='Commit and Tag'></b>
              </td>
          </tr>\n";

    echo "</form>
          </table>\n";

    echo "<blockquote><blockquote><blockquote>
          <ol>
            <li> Optional tag must contain only alphanumeric characters,
                 starting with an alpha character. If no tag is supplied,
                 we will make one up for you, but that is probably not
                 what you want.

          </ol>
          </blockquote></blockquote></blockquote>\n";
}

#
# On first load, display a virgin form and exit.
#
if (! isset($submit)) {
    $defaults = array();
    $defaults["tag"]     = "";
    $defaults["message"] = "";
    
    if (!isset($referrer))
	$referrer = $_SERVER['HTTP_REFERER'];
    
    SPITFORM($defaults, 0);
    PAGEFOOTER();
    return;
}

# Args to shell.
$tag      = "";
$tagarg   = "";
$message  = "";
$tmpfname = Null;

function CLEANUP()
{
    global $tmpfname;

    if ($tmpfname != NULL)
	unlink($tmpfname);
    
    exit();
}
register_shutdown_function("CLEANUP");

#
# Otherwise, must validate and redisplay if errors
#
$errors = array();

#
# Tag
#
if (isset($formfields["tag"]) && $formfields["tag"] != "") {
    if (! TBvalid_archive_tag($formfields["tag"])) {
	$errors["Tag"] = TBFieldErrorString();
    }
    else {
	$tag = escapeshellarg($formfields["tag"]);
	$tagarg = "-t $tag" ;
    }
}

if (isset($formfields["message"]) && $formfields["message"] != "") {
    if (! TBvalid_archive_message($formfields["message"])) {
	$errors["Message"] = TBFieldErrorString();
    }
    else {
	#
	# Easier to stick the entire message into a temporary file and
	# send that through. 
	#
	$tmpfname = tempnam("/tmp", "archive_tag");

	$fp = fopen($tmpfname, "w");
	fwrite($fp, $formfields["message"]);
	fclose($fp);
	chmod($tmpfname, 0666);
	
	$message = "-m $tmpfname";
    }
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

STARTBUSY("Committing and Tagging!");

#
# First lets make sure the tag is unique. 
#
if ($tag != "") {
    $retval = SUEXEC($uid, "$pid,$gid",
		     "webarchive_control checktag $pid $eid $tag",
		     SUEXEC_ACTION_IGNORE);

    /* Clear the various 'loading' indicators. */
    if ($retval) 
	CLEARBUSY();

    #
    # Fatal Error. 
    # 
    if ($retval < 0) {
	SUEXECERROR(SUEXEC_ACTION_DIE);
    }

    # User error. Tell user and exit.
    if ($retval) {
	$errors["Tag"] = "Already in use; pick another";
    
	SPITFORM($formfields, $errors);
	PAGEFOOTER();
	return;
    }
}

SUEXEC($uid, "$pid,$gid",
       "webarchive_control $tagarg $message -u commit $pid $eid",
       SUEXEC_ACTION_DIE);

STOPBUSY();

if (!isset($referrer)) {
    $exptidx  = $experiment->idx();
    $referrer = "archive_view.php3/$exptidx/?exptidx=$exptidx";
}

PAGEREPLACE($referrer);

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
