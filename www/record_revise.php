<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Only known and logged in users can look at experiments.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$reqargs = RequiredPageArguments("instance",   PAGEARG_INSTANCE,
				 "runidx",     PAGEARG_INTEGER);
$optargs = OptionalPageArguments("submit",     PAGEARG_STRING,
				 "referrer",   PAGEARG_STRING,
				 "formfields", PAGEARG_ARRAY);

$template = $instance->GetTemplate();

# Need these below.
$pid = $instance->pid();
$eid = $instance->eid();
$gid = $instance->gid();

# Permission
if (!$isadmin &&
    !$instance->AccessCheck($this_user, $TB_EXPT_MODIFY)) {
    USERERROR("You do not have permission to change this record!", 1);
}

#
# Standard Testbed Header
#
PAGEHEADER("Revise Run Record");

function SPITFORM($formfields, $errors)
{
    global $instance, $runidx, $TBDB_ARCHIVE_TAGLEN, $referrer;

    $iid   = $instance->id();
    $runid = $instance->GetRunID($runidx);

    echo $instance->RunPageHeader($runidx);

    echo "<br><br><center>
          Revise Record $runid in Instance $iid </center><br>\n";

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

    $url = CreateURL("record_revise", $instance, "runidx", $runidx);

    echo "<table align=center border=1> 
          <form action='$url' method=post>\n";

    if (isset($referrer)) {
	echo "<input type=hidden name=referrer value=$referrer>\n";
    }
    
    echo "<tr>
              <td align=center>
               <b>Please enter a message to be logged
                       with the revision</b>
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
                 <b><input type=submit name=submit value='Revise Record'></b>
              </td>
          </tr>\n";

    echo "</form>
          </table>\n";

    echo "<blockquote><blockquote>
          <ul>
            <li> This operation will update the end of run archive that was
                 saved when the run originally completed. The original record
                 will not be lost, and you can update as many times as you
                 like.</li>
          </ul>
          </blockquote></blockquote>\n";
}

#
# On first load, display a virgin form and exit.
#
if (! isset($submit)) {
    $defaults = array();
    $defaults["message"] = "";
    
    if (!isset($referrer))
	$referrer = $_SERVER['HTTP_REFERER'];
    
    SPITFORM($defaults, 0);
    PAGEFOOTER();
    return;
}

# Args to shell.
$command  = "-r $runidx -i " . $instance->exptidx();
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
else {
    $errors["Message"] = "Please provide a minimal message!";
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

STARTBUSY("Revising the record");

SUEXEC($uid, "$pid,$gid",
       "webtemplate_revise -t ReviseRecord $message $command ",
       SUEXEC_ACTION_DIE);

STOPBUSY();

if (!isset($referrer)) {
    $referrer = CreateURL("instance_show", $instance);
}

PAGEREPLACE($referrer);

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
