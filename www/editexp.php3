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
    global $isadmin, $TBDOCBASE, $linktest_levels, $EXPOSELINKTEST;

    #
    # Standard Testbed Header
    #
    PAGEHEADER("Edit Experiment Settings");

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
    if ($isadmin) {
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
# Construct a defaults array based on current DB info. Used for the initial
# form, and to determine if any changes were made.
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
# On first load, display initial values and exit.
#
if (! isset($submit)) {
    SPITFORM($experiment, $defaults, 0);
    PAGEFOOTER();
    return;
}

#
# Build up argument array to pass along.
#
$args = array();

# Skip passing ones that are not changing from the default (DB state.)
if (isset($formfields["description"]) && $formfields["description"] != "" &&
    ($formfields["description"] != $experiment->description())) {
    $args["description"] = $formfields["description"];
}

if ($isadmin) {			# A couple of admin-only options.
    # Filter booleans from checkboxes to 0 or 1.
    $formfields["idle_ignore"] = 
	(!isset($formfields["idle_ignore"]) ||
	 strcmp($formfields["idle_ignore"], "1")) ? 0 : 1;
    if ($formfields["idle_ignore"] != $experiment->idle_ignore()) {
	$args["idle_ignore"] = $formfields["idle_ignore"];
    }

    $formfields["swappable"] = (!isset($formfields["swappable"]) ||
				strcmp($formfields["swappable"], "1")) ? 0 : 1;
    if ($formfields["swappable"] != $experiment->swappable()) {
	$args["swappable"] = $formfields["swappable"];
    }
    if (isset($formfields["noswap_reason"]) &&
	$formfields["noswap_reason"] != "" &&
	($formfields["noswap_reason"] != $experiment->noswap_reason())) {
	$args["noswap_reason"] = $formfields["noswap_reason"];
    }
}

$formfields["idleswap"] = (!isset($formfields["idleswap"]) ||
			   strcmp($formfields["idleswap"], "1")) ? 0 : 1;
if ($formfields["idleswap"] != $experiment->idleswap()) {
    $args["idleswap"] = $formfields["idleswap"];
}
# Note that timeouts are in hours in the UI, but in minutes in the DB. 
if (isset($formfields["idleswap_timeout"]) && 
    $formfields["idleswap_timeout"] != "" &&
    ($formfields["idleswap_timeout"] != 
     $experiment->idleswap_timeout() / 60.0)) {
    $args["idleswap_timeout"] = $formfields["idleswap_timeout"];
}
if (isset($formfields["noidleswap_reason"]) && 
    $formfields["noidleswap_reason"] != "" &&
    ($formfields["noidleswap_reason"] != $experiment->noidleswap_reason())) {
    $args["noidleswap_reason"] = $formfields["noidleswap_reason"];
}

$formfields["autoswap"] = (!isset($formfields["autoswap"]) ||
			   strcmp($formfields["autoswap"], "1")) ? 0 : 1;
if ($formfields["autoswap"] != $experiment->autoswap()) {
    $args["autoswap"] = $formfields["autoswap"];
}
if (isset($formfields["autoswap_timeout"]) && 
    $formfields["autoswap_timeout"] != "" &&
    ($formfields["autoswap_timeout"] != 
     $experiment->autoswap_timeout() / 60.0)) {
    $args["autoswap_timeout"] = $formfields["autoswap_timeout"];
}

$formfields["savedisk"] = (!isset($formfields["savedisk"]) ||
			   strcmp($formfields["savedisk"], "1")) ? 0 : 1;
if ($formfields["savedisk"] != $experiment->savedisk()) {
    $args["savedisk"] = $formfields["savedisk"];
}

if (isset($formfields["cpu_usage"]) && $formfields["cpu_usage"] != "" &&
    ($formfields["cpu_usage"] != $experiment->cpu_usage())) {
    $args["cpu_usage"] = $formfields["cpu_usage"];
}

if (isset($formfields["mem_usage"]) && $formfields["mem_usage"] != "" &&
    ($formfields["mem_usage"] != $experiment->mem_usage())) {
    $args["mem_usage"] = $formfields["mem_usage"];
}

$formfields["batchmode"] = (!isset($formfields["batchmode"]) ||
			    strcmp($formfields["batchmode"], "1")) ? 0 : 1;
if ($formfields["batchmode"] != $experiment->batchmode()) {
    $args["batchmode"] = $formfields["batchmode"];
}

# Select defaults to "none" if not set.
if (isset($formfields["linktest_level"]) &&
    $formfields["linktest_level"] != "none" && 
    $formfields["linktest_level"] != "" &&
    ($formfields["linktest_level"] != $experiment->linktest_level())) {
    $args["linktest_level"] = $formfields["linktest_level"];
}

$errors  = array();
if (! ($result = Experiment::EditExp($experiment, $args, $errors))) {
    # Always respit the form so that the form fields are not lost.
    # I just hate it when that happens so lets not be guilty of it ourselves.
    SPITFORM($experiment, $formfields, $errors);
    PAGEFOOTER();
    return;
}

#
# Spit out a redirect so that the history does not include a post
# in it. The back button skips over the post and to the form.
#
header("Location: " . CreateURL("showexp", $experiment));

?>
