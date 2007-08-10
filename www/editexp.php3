<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# No PAGEHEADER since we spit out a Location header later. See below.
#

#
# Only known and logged in users can end experiments.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();
$idleswaptimeout = TBGetSiteVar("idle/threshold");

#
# Verify page arguments.
#
$reqargs = RequiredPageArguments("experiment", PAGEARG_EXPERIMENT);
$optargs = OptionalPageArguments("submit",     PAGEARG_STRING,
				 "formfields", PAGEARG_ARRAY);

# Need below
$pid = $experiment->pid();
$eid = $experiment->eid();

#
# Verify Permission.
#
if (! $experiment->AccessCheck($this_user, $TB_EXPT_MODIFY)) {
    USERERROR("You do not have permission to modify experiment $pid/$eid!", 1);
}

#
# Spit the form out using the array of data.
#
function SPITFORM($experiment, $formfields, $errors)
{
    global $TBDOCBASE, $linktest_levels, $EXPOSELINKTEST;

    #
    # Standard Testbed Header
    #
    PAGEHEADER("Edit Experiment Metadata");

    echo $experiment->PageHeader();
    echo "<br><br>\n";

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

    $url = CreateURL("editexp", $experiment);
    echo "<table align=center border=1>
          <form action='$url' method=post>\n";

    echo "<tr>
             <td>Description:</td>
             <td class=left>
                 <input type=text
                        name=\"formfields[description]\"
                        value='" . htmlspecialchars($formfields['description'],
						    ENT_QUOTES) . "'
	                size=30>
             </td>
          </tr>\n";

    #
    # Swapping goo.
    #
    $swaplink = "$TBDOCBASE/docwrapper.php3?docname=swapping.html";

    echo "<tr>
	      <td class='pad4'>
		  <a href='${swaplink}#swapping'>Swapping:
              </td>
	      <td>
	          <table cellpadding=0 cellspacing=0 border=0>\n";
    if (ISADMIN()) {
        #
        # Batch Experiment?
        #
	echo "    <tr>
  	              <td>
                           <input type=checkbox
                                  name='formfields[idle_ignore]'
                                  value=1";

	if (isset($formfields["idle_ignore"]) &&
	    strcmp($formfields["idle_ignore"], "1") == 0) {
	    echo " checked='1'";
	}

	echo ">
                      </td>
                      <td>
	                   Idle Ignore
                      </td>
                  </tr>\n";

	echo "    <tr>
                       <td>
                          <input type='checkbox'
	                         name='formfields[swappable]'
	                         value='1'";

	if ($formfields["swappable"] == "1") {
	    echo " checked='1'";
	}
	echo ">
                      </td>
                      <td>
                          <a href='{$swaplink}#swapping'>
	                     <b>Swappable:</b></a>
                             This experiment can be swapped.
                      </td>
	              </tr>
	              <tr>
                      <td></td>
   	              <td>If not, why not (administrators option)?<br>
                          <textarea rows=2 cols=50
                                    name='formfields[noswap_reason]'>" .
	                    htmlspecialchars($formfields["noswap_reason"],
					     ENT_QUOTES) .
	                 "</textarea>
                      </td>
	              </tr><tr>\n";
    }
    echo "            <td>
                      <input type='checkbox'
	                     name='formfields[idleswap]'
	                     value='1'";

    if ($formfields["idleswap"] == "1") {
	echo " checked='1'";
    }
    echo ">
                  </td>
                  <td>
                      <a href='{$swaplink}#idleswap'>
	                 <b>Idle-Swap:</b></a>
                         Swap out this experiment after
                         <input type='text'
                                name='formfields[idleswap_timeout]'
		                value='" . $formfields["idleswap_timeout"] . "'
                                size='3'>
                         hours idle.
                  </td>
	          </tr>
	          <tr>
                  <td></td>
   	          <td>If not, why not?<br>
                      <textarea rows=2 cols=50
                                name='formfields[noidleswap_reason]'>" .
	                    htmlspecialchars($formfields["noidleswap_reason"],
					     ENT_QUOTES) .
	             "</textarea>
                  </td>
	          </tr><tr>
  	          <td>
                      <input type='checkbox'
		             name='formfields[autoswap]'
		             value='1' ";

    if ($formfields["autoswap"] == "1") {
	echo " checked='1'";
    }
    echo ">
                  </td>
	          <td>
                      <a href='${swaplink}#autoswap'>
	                 <b>Max. Duration:</b></a>
                      Swap out after
                        <input type='text'
                               name='formfields[autoswap_timeout]'
		               value='" . $formfields["autoswap_timeout"] . "'
                               size='3'>
                      hours, even if not idle.
                  </td>
                  </tr>";

    #
    # Swapout disk state saving
    # XXX requires more work, need to create/remove sigs as appropriate
    #
    if (0) {
    echo "<tr>
		  <td>
	              <input type='checkbox'
		             name='formfields[savedisk]'
	                     value='1' ";

    if ($formfields["savedisk"] == "1") {
	echo " checked='1'";
    }
    echo ">
                  </td>
                  <td>
                      <a href='${swaplink}#swapstatesave'>
		         <b>State Saving:</b></a>
                      Save disk state on swapout
		  </td>
	       </tr>";
    }

    echo "</table></td></tr>";

    #
    # Resource usage.
    #
    echo "<tr>
              <td class='pad4'>CPU Usage:</td>
              <td class=left>
                  <input type=text
                         name=\"formfields[cpu_usage]\"
                         value=\"" . $formfields["cpu_usage"] . "\"
	                 size=2>
                  (PlanetLab Nodes Only: 1 &lt= X &lt= 5)
              </td>
          </tr>\n";

    echo "<tr>
              <td class='pad4'>Mem Usage:</td>
              <td class=left>
                  <input type=text
                         name=\"formfields[mem_usage]\"
                         value=\"" . $formfields["mem_usage"] . "\"
	                 size=2>
                  (PlanetLab Nodes Only: 1 &lt= X &lt= 5)
              </td>
          </tr>\n";

    #
    # Batch Experiment?
    #
    echo "<tr>
  	      <td class=left colspan=2>
              <input type=checkbox name='formfields[batchmode]' value='1'";

    if (isset($formfields["batchmode"]) &&
	strcmp($formfields["batchmode"], "1") == 0) {
	    echo " checked='1'";
    }

    echo ">\n";
    echo "Batch Mode Experiment &nbsp;
          <font size='-1'>(See
          <a href='$TBDOCBASE/tutorial/tutorial.php3#BatchMode'>Tutorial</a>
          for more information)</font>
          </td>
          </tr>\n";

    #
    # Run linktest, and level. 
    # 
    if (STUDLY() || $EXPOSELINKTEST) {
    echo "<tr>
              <td><a href='$TBDOCBASE/doc/docwrapper.php3?".
	                  "docname=linktest.html'>Linktest</a> Option:</td>
              <td><select name=\"formfields[linktest_level]\">
                          <option value=0>Skip Linktest </option>\n";

    for ($i = 1; $i <= TBDB_LINKTEST_MAX; $i++) {
	$selected = "";

	if (strcmp($formfields["linktest_level"], "$i") == 0)
	    $selected = "selected";
	
	echo "        <option $selected value=$i>Level $i - " .
	    $linktest_levels[$i] . "</option>\n";
    }
    echo "       </select>";
    echo "    (<a href='$TBDOCBASE/doc/docwrapper.php3?".
	"docname=linktest.html'><b>What is this?</b></a>)";
    echo "    </td>
          </tr>\n";
    }

    echo "<tr>
              <td colspan=2 align=center>
                 <b><input type=submit name=submit value=Submit></b>
              </td>
          </tr>\n";

    echo "</form>
          </table>\n";
}

#
# We might need these later for email.
#
$creator = $experiment->creator();
$swapper = $experiment->swapper();
$doemail = 0;

#
# Construct a defaults array based on current DB info. Used for the initial
# form, and to determine if any changes were made and to send email.
#
$defaults                      = array();
$defaults["description"]       = $experiment->description();
$defaults["idle_ignore"]       = $experiment->idle_ignore();
$defaults["batchmode"]         = $experiment->batchmode();
$defaults["swappable"]         = $experiment->swappable();
$defaults["noswap_reason"]     = $experiment->noswap_reason();
$defaults["idleswap"]          = $experiment->idleswap();
$defaults["idleswap_timeout"]  = $experiment->idleswap_timeout() / 60.0;
$defaults["noidleswap_reason"] = $experiment->noidleswap_reason();
$defaults["autoswap"]          = $experiment->autoswap();
$defaults["autoswap_timeout"]  = $experiment->autoswap_timeout() / 60.0;
$defaults["savedisk"]          = $experiment->savedisk();
$defaults["mem_usage"]         = $experiment->mem_usage();
$defaults["cpu_usage"]         = $experiment->cpu_usage();
$defaults["linktest_level"]    = $experiment->linktest_level();

#
# A couple of defaults for turning things on.
#
if (!$defaults["autoswap"]) {
     $defaults["autoswap_timeout"] = 10;
}
if (!$defaults["idleswap"]) {
     $defaults["idleswap_timeout"] = $idleswaptimeout;
}

#
# On first load, display initial values.
#
if (! isset($submit)) {
    SPITFORM($experiment, $defaults, 0);
    PAGEFOOTER();
    return;
}

#
# Otherwise, must validate and redisplay if errors. Build up a DB insert
# string as we go.
#
$errors  = array();
$inserts = array();

#
# Description
#
if (!isset($formfields["description"]) ||
    strcmp($formfields["description"], "") == 0) {
    $errors["Description"] = "Missing Field";
}
else {
    $inserts[] = "expt_name='" . addslashes($formfields["description"]) . "'";
}

#
# Swappable/Idle Ignore
# Any of these which are not "1" become "0".
#
# Idle Ignore
#
if (!isset($formfields["idle_ignore"]) ||
    strcmp($formfields["idle_ignore"], "1")) {
    $formfields["idle_ignore"] = 0;
    $inserts[] = "idle_ignore=0";
}
else {
    $formfields["idle_ignore"] = 1;
    $inserts[] = "idle_ignore=1";
}

#
# Swappable
#
if (ISADMIN() && (!isset($formfields["swappable"]) ||
    strcmp($formfields["swappable"], "1"))) {
    $formfields["swappable"] = 0;

    if (!isset($formfields["noswap_reason"]) ||
        !strcmp($formfields["noswap_reason"], "")) {

        if (!ISADMIN()) {
	    $errors["Swappable"] = "No justification provided";
        }
	else {
	    $formfields["noswap_reason"] = "ADMIN";
        }
    }
    if ($defaults["swappable"])
	$doemail = 1;
    $inserts[] = "swappable=0";
    $inserts[] = "noswap_reason='" .
	         addslashes($formfields["noswap_reason"]) . "'";
}
else {
    $inserts[] = "swappable=1";
}

#
# IdleSwap
#
if (!isset($formfields["idleswap"]) ||
    strcmp($formfields["idleswap"], "1")) {
    $formfields["idleswap"] = 0;

    if (!isset($formfields["noidleswap_reason"]) ||
	!strcmp($formfields["noidleswap_reason"], "")) {

	if (! ISADMIN()) {
	    $errors["IdleSwap"] = "No justification provided";
	}
	else {
	    $formfields["noidleswap_reason"] = "ADMIN";
	}
    }
    if ($defaults["idleswap"])
	$doemail = 1;
    $inserts[] = "idleswap=0";
    $inserts[] = "idleswap_timeout=0";
    $inserts[] = "noidleswap_reason='" .
	         addslashes($formfields["noidleswap_reason"]) . "'";
}
elseif (!isset($formfields["idleswap_timeout"]) ||
	!preg_match("/^[\d]+$/", $formfields["idleswap_timeout"]) ||
	($formfields["idleswap_timeout"] + 0) <= 0 ||
	( (($formfields["idleswap_timeout"] + 0) > $idleswaptimeout) &&
	  !ISADMIN()) ) {
    $errors["Idleswap"] = "Invalid time provided (0 < X <= $idleswaptimeout)";
}
else {
    $inserts[] = "idleswap=1";
    $inserts[] = "idleswap_timeout=" . 60 * $formfields["idleswap_timeout"];
    $inserts[] = "noidleswap_reason='" .
	         addslashes($formfields["noidleswap_reason"]) . "'";
}

#
# AutoSwap
#
if (!isset($formfields["autoswap"]) ||
    strcmp($formfields["autoswap"], "1")) {
    $formfields["autoswap"] = 0;
    $inserts[] = "autoswap=0";
    $inserts[] = "autoswap_timeout=0";
}
elseif (!isset($formfields["autoswap_timeout"]) ||
	!preg_match("/^[\d]+$/", $formfields["autoswap_timeout"]) ||
	($formfields["autoswap_timeout"] + 0) == 0) {
    $errors["Max Duration"] = "Invalid time provided";
}
else {
    $inserts[] = "autoswap=1";
    $inserts[] = "autoswap_timeout=" . 60 * $formfields["autoswap_timeout"];
}

#
# Swapout disk state saving
#
if (!isset($formfields["savedisk"]) ||
    strcmp($formfields["savedisk"], "1")) {
    $formfields["savedisk"] = 0;
    $inserts[] = "savedisk=0";
}
else {
    $formfields["savedisk"] = 1;
    $inserts[] = "savedisk=1";
}

#
# CPU Usage
#
if (isset($formfields["cpu_usage"]) &&
    strcmp($formfields["cpu_usage"], "")) {

    if (!preg_match("/^[\d]+$/", $formfields["cpu_usage"])) {
	$errors["CPU Usage"] = "Invalid character";
    }
    elseif (($formfields["cpu_usage"] + 0) < 0 ||
	($formfields["cpu_usage"] + 0) > 5) {
	$errors["CPU Usage"] = "Invalid (0 <= X <= 5)";
    }
    else {
	$inserts[] = "cpu_usage=" . $formfields["cpu_usage"];
    }
}
else {
    $inserts[] = "cpu_usage=0";
}

#
# Mem Usage
#
if (isset($formfields["mem_usage"]) &&
    strcmp($formfields["mem_usage"], "")) {

    if (!preg_match("/^[\d]+$/", $formfields["mem_usage"])) {
	$errors["Mem Usage"] = "Invalid character";
    }
    elseif (($formfields["mem_usage"] + 0) < 0 ||
	($formfields["mem_usage"] + 0) > 5) {
	$errors["Mem Usage"] = "Invalid (0 <= X <= 5)";
    }
    else {
	$inserts[] = "mem_usage=" . $formfields["mem_usage"];
    }
}
else {
    $inserts[] = "mem_usage=0";
}

#
# Linktest level
#
if (isset($formfields["linktest_level"]) &&
    strcmp($formfields["linktest_level"], "")) {

    if (!preg_match("/^[\d]+$/", $formfields["linktest_level"])) {
	$errors["Linktest Level"] = "Invalid character";
    }
    elseif (($formfields["linktest_level"] + 0) < 0 ||
	($formfields["linktest_level"] + 0) > 4) {
	$errors["Linktest Level"] = "Invalid (0 <= X <= 4)";
    }
    else {
	$inserts[] = "linktest_level=" . $formfields["linktest_level"];
    }
}
else {
    $inserts[] = "linktest_level=0";
}

#
# Spit any errors before dealing with batchmode, which changes the DB.
#
if (count($errors)) {
    SPITFORM($experiment, $formfields, $errors);
    PAGEFOOTER();
    return;
}

#
# Converting the batchmode is tricky, but we can let the DB take care
# of it by requiring that the experiment not be locked, and it be in
# the swapped state. If the query fails, we know that the experiment
# was in transition.
#
if (!isset($formfields["batchmode"])) {
    $formfields["batchmode"] = 0;
}
if ($defaults["batchmode"] != $formfields["batchmode"]) {
    $success  = 0;

    if (strcmp($formfields["batchmode"], "1")) {
	$batchmode = 0;
	$formfields["batchmode"] = 0;
    }
    else {
	$batchmode = 1;
	$formfields["batchmode"] = 1;
    }
    if ($experiment->SetBatchMode($batchmode) != 0) {
	$errors["Batch Mode"] = "Experiment is running or in transition; ".
	    "try again later";

	SPITFORM($experiment, $formfields, $errors);
	PAGEFOOTER();
	return;
    }
}

#
# Otherwise, do the other inserts.
#
if ($experiment->UpdateOldStyle($inserts) != 0) {
    $errors["Updating"] = "Error updating experiment; please try again later";
    SPITFORM($experiment, $formfields, $errors);
    PAGEFOOTER();
    return;
}

#
# Do not send this email if the user is an administrator
# (adminmode does not matter), and is changing an expt he created
# or swapped in. Pointless email.
if ($doemail &&
    ! (ISADMINISTRATOR() &&
       (!strcmp($uid, $creator) || !strcmp($uid, $swapper)))) {

    $target_creator = $experiment->GetCreator();
    $target_swapper = $experiment->GetSwapper();
    # Needed below.
    $swappable          = $experiment->swappable();
    $idleswap           = $experiment->idleswap();
    $noswap_reason      = $experiment->noswap_reason();
    $idleswap_timeout   = $experiment->idleswap_timeout() / 60.0;
    $noidleswap_reason  = $experiment->noidleswap_reason();
    $autoswap           = $experiment->autoswap();
    $autoswap_timeout   = $experiment->autoswap_timeout() / 60.0;

    $user_name  = $this_user->name();
    $user_email = $this_user->email();
    $cname      = $target_creator->name();
    $cemail     = $target_creator->email();
    $sname      = $target_swapper->name();
    $semail     = $target_swapper->email();

    $olds = ($defaults["swappable"] ? "Yes" : "No");
    $oldsr= $defaults["noswap_reason"];
    $oldi = ($defaults["idleswap"] ? "Yes" : "No");
    $oldit= $defaults["idleswap_timeout"];
    $oldir= $defaults["noidleswap_reason"];
    $olda = ($defaults["autoswap"] ? "Yes" : "No");
    $oldat= $defaults["autoswap_timeout"];

    $s    = ($formfields["swappable"] ? "Yes" : "No");
    $sr   = $formfields["noswap_reason"];
    $i    = ($formfields["idleswap"] ? "Yes" : "No");
    $it   = $formfields["idleswap_timeout"];
    $ir   = $formfields["noidleswap_reason"];
    $a    = ($formfields["autoswap"] ? "Yes" : "No");
    $at   = $formfields["autoswap_timeout"];

    TBMAIL($TBMAIL_OPS,"$pid/$eid swap settings changed",
	   "\nThe swap settings for $pid/$eid have changed.\n".
	   "\nThe old settings were:\n".
	   "Swappable:\t$olds\t($oldsr)\n".
	   "Idleswap:\t$oldi\t(after $oldit hrs)\t($oldir)\n".
	   "MaxDuration:\t$olda\t(after $oldat hrs)\n".
	   "\nThe new settings are:\n".
	   "Swappable:\t$s\t($sr)\n".
	   "Idleswap:\t$i\t(after $it hrs)\t($ir)\n".
	   "MaxDuration:\t$a\t(after $at hrs)\n".
	   "\nCreator:\t$creator ($cname <$cemail>)\n".
	   "Swapper:\t$swapper ($sname <$semail>)\n".
	   "\nIf it is necessary to change these settings, ".
	   "please reply to this message \nto notify the user, ".
	   "then change the settings here:\n\n".
	   "$TBBASE/showexp.php3?pid=$pid&eid=$eid\n\n".
	   "Thanks,\nTestbed WWW\n",
	   "From: $user_name <$user_email>\n".
	   "Errors-To: $TBMAIL_WWW");
}

#
# Spit out a redirect so that the history does not include a post
# in it. The back button skips over the post and to the form.
#
header("Location: " . CreateURL("showexp", $experiment));

?>
