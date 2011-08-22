<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006-2011 University of Utah and the Flux Group.
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

# This will not return if its a sajax request.
include("showlogfile_sup.php3");

#
# Verify page arguments
#
$reqargs = RequiredPageArguments("template",   PAGEARG_TEMPLATE);
$optargs = OptionalPageArguments("swapin",     PAGEARG_STRING,
				 "formfields", PAGEARG_ARRAY,
				 "parameters", PAGEARG_ARRAY,
				 "replay_instance_idx", PAGEARG_INTEGER,
				 "replay_run_idx",      PAGEARG_INTEGER);

# Need these below.
$guid = $template->guid();
$vers = $template->vers();
$pid  = $template->pid();
$unix_gid = $template->UnixGID();
$project  = $template->GetProject();
$unix_pid = $project->unix_gid();

if (! $template->AccessCheck($this_user, $TB_EXPT_MODIFY)) {
    USERERROR("You do not have permission to export in template ".
	      "$guid/$vers!", 1);
}

# Used below
unset($parameter_xmlfile);
$deletexmlfile = 0;
$batchmode     = 0;
$preload       = 0;

#
# Spit the form out using the array of data.
#
function SPITFORM($template, $formfields, $parameters, $errors)
{
    global $TBDB_EIDLEN, $EXPOSELINKTEST, $EXPOSESTATESAVE, $TBDOCBASE;
    global $TBVALIDDIRS_HTML, $linktest_levels;
    global $WIKIDOCURL;

    PAGEHEADER("Instantiate an Experiment Template");

    echo "<center>\n";
    $template->Show();
    echo "</center>\n";
    echo "<br>\n";

    $guid = $template->guid();
    $vers = $template->vers();

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
	echo "<blockquote><blockquote><font size=+1>";
	echo "Template Instantiation will map your template onto actual ".
	    "testbed hardware. This used to be known as ".
	    "<em>experiment swapin</em> ".
	    "but we decided we like <em>instantiation</em> better.";
	echo "</font></blockquote></blockquote><br>\n";
    }

    $url = CreateURL("template_swapin", $template);
    echo "<form action='$url' method=post>\n";
    echo "<table align=center border=1>\n";

    #
    # Pass along replay info, but no need to display it.
    #
    if (isset($formfields["replay_instance_idx"])) {
	echo "<input type=hidden name=\"formfields[replay_instance_idx]\"
                     value='" . $formfields["replay_instance_idx"] . "'>\n";

	if (isset($formfields["replay_run_idx"])) {
	    echo "<input type=hidden name=\"formfields[replay_run_idx]\"
                         value='" . $formfields["replay_run_idx"] . "'>\n";
	}
    }

    #
    # EID:
    #
    echo "<tr>
              <td class='pad4'>ID:
              <br><font size='-1'>(alphanumeric, no blanks)</font></td>
              <td class='pad4' class=left>
                  <input type=text
                         name=\"formfields[eid]\"
                         value=\"" . $formfields["eid"] . "\"
	                 size=$TBDB_EIDLEN
                         maxlength=$TBDB_EIDLEN>
              </td>
          </tr>\n";

    #
    # Swapping
    #
    # Add in hidden fields to send swappable and noswap_reason, since
    # they do not show on the form
    echo "<input type=hidden name=\"formfields[exp_swappable]\"
                 value='$formfields[exp_swappable]'>\n";
    echo "<input type=hidden name=\"formfields[exp_noswap_reason]\" value='";
    echo htmlspecialchars($formfields["exp_noswap_reason"], ENT_QUOTES);
    echo "'>\n";
    
    if (isset($view['hide_swap'])) {
	$idlevars = array('exp_idleswap','exp_noidleswap_reason',
			  'exp_idleswap_timeout',
	                  'exp_autoswap','exp_autoswap_timeout');
	while (list($index,$value) = each($idlevars)) {
	    if (isset($formfields[$value])) {
		echo "<input type='hidden' name='formfields[$value]'
                             value='$formfields[$value]'>\n";
	    }
	}
    }
    else {
	echo "<tr>
		  <td class='pad4'>
		    <a href='$WIKIDOCURL/Swapping#swapping'>
		    Swapping:</td>
		  <td>
		  <table cellpadding=0 cellspacing=0 border=0><tr>
		  <td><input type='checkbox'
			 name='formfields[exp_idleswap]'
			 value='1'";
	if ($formfields["exp_idleswap"] == "1") {
	    echo " checked='1'";
	}
	echo "></td>
		  <td><a href='$WIKIDOCURL/Swapping#idleswap'>
		  <b>Idle-Swap:</b></a> Swap out this experiment
		  after 
		  <input type='text' name='formfields[exp_idleswap_timeout]'
			 value='";
	echo htmlspecialchars($formfields["exp_idleswap_timeout"], ENT_QUOTES);
	echo "' size='3'> hours idle.</td>
		  </tr><tr>
		  <td> </td>
		  <td>If not, why not?<br><textarea rows=2 cols=50
			      name='formfields[exp_noidleswap_reason]'>";
			      
	echo htmlspecialchars($formfields["exp_noidleswap_reason"],ENT_QUOTES);
	echo "</textarea></td>
		  </tr><tr>
		  <td><input type='checkbox'
			 name='formfields[exp_autoswap]'
			 value='1' ";
	if ($formfields["exp_autoswap"] == "1") {
	    echo " checked='1'";
	}
	echo "></td>
		  <td><a href='$WIKIDOCURL/Swapping#autoswap'>
		  <b>Max. Duration:</b></a> Swap out after
		  <input type='text' name='formfields[exp_autoswap_timeout]'
			 value='";
	echo htmlspecialchars($formfields["exp_autoswap_timeout"], ENT_QUOTES);
	echo "' size='3'> hours, even if not idle.</td>
		  </tr>";

	if (STUDLY() || $EXPOSESTATESAVE) {
	    echo "<tr><td>
	         <input type=checkbox name='formfields[exp_savedisk]'
	         value='Yep'";

	    if (isset($formfields["exp_savedisk"]) &&
		strcmp($formfields["exp_savedisk"], "Yep") == 0) {
		    echo " checked='1'";
	    }

	    echo "></td>\n";
	    echo "<td><a href='$WIKIDOCURL/Swapping#swapstatesave'>
		  <b>State Saving:</b></a> Save disk state on swapout</td>
		  </tr>";
	
	}
	echo "</table></td></tr>";
    }
    
    if (count($parameters)) {
	#
	# Lets get an array of mouseovers to use in the form.
	#
	unset($mouseovers);
	$template->FormalParameterMouseOvers($mouseovers);
	
        #
	# Table of inputs.
	#
	echo "<tr>
		  <td class='pad4'>Formal Parameters:</td>
		  <td>
 		    <table cellpadding=0 cellspacing=0 border=0>\n";
	
	while (list ($name, $value) = each ($parameters)) {
	    if (!isset($value))
		$value = "&nbsp";
	    $mouseover = (isset($mouseovers[$name]) ? $mouseovers[$name] : "");

	    echo "<tr>
                    <td class='pad4' $mouseover>$name</td>
                    <td class='pad4' class=left>
                        <input type=text
                               name=\"parameters[$name]\"
                               value=\"" . $value . "\"
	                       size=60
                               maxlength=1024>
                    </td>
                  </tr>\n";
	}
	echo "<tr><td>&nbsp;&nbsp;<b>or</b> XML file:
               </td><td></td></tr>\n";
	echo "<tr>
                  <td class='pad4'>On Server<br>
                           <font size='-1'>($TBVALIDDIRS_HTML)</font></td>
                  <td class='pad4'>
	              <input type=text
                             name=\"formfields[parameter_xmlfile]\"
                             value=\"" . $formfields["parameter_xmlfile"] . "\"
	                     size=60>\n";
	echo "</td></tr>\n";
	echo "</table></td></tr>";
    }

    #
    # Description
    #
    echo "<tr>
              <td colspan=2>
               Use this text area for an (optional) description:
              </td>
          </tr>
          <tr>
              <td colspan=2 align=center class=left>
                  <textarea name=\"formfields[description]\"
                    rows=5 cols=80>" .
	            ereg_replace("\r", "", $formfields["description"]) .
	           "</textarea>
              </td>
          </tr>\n";

    #
    # Batch Experiment?
    #
    echo "<tr>
	      <td class='pad4' colspan=2>
	      <input type=checkbox name='formfields[batched]' value='Yep'";

    if (isset($formfields["batched"]) &&
	strcmp($formfields["batched"], "Yep") == 0) {
	echo " checked='1'";
    }
    echo ">\n";
    echo "Batch Mode Instantiation &nbsp;
	  <font size='-1'>(See
          <a href='$WIKIDOCURL/Tutorial#BatchMode'>Tutorial</a>
          for more information)</font>
	  </td>
	  </tr>\n";

    #
    # Preload?
    #
    echo "<tr>
	      <td class='pad4' colspan=2>
		  <input type=checkbox name='formfields[preload]'
                         value='Yep'";

    if (isset($formfields["preload"]) &&
	strcmp($formfields["preload"], "Yep") == 0) {
	echo " checked='1'";
    }
    echo ">\n";
    echo "Do Not Swap In</td>
	 </tr>\n";

    #
    # Run linktest, and level. 
    #
    if (STUDLY() || $EXPOSELINKTEST) {
	echo "<tr>
              <td><a href='$WIKIDOCURL/linktest'>Linktest</a> Option:</td>
              <td><select name=\"formfields[exp_linktest]\">
                          <option value=0>Skip Linktest </option>\n";

	for ($i = 1; $i <= TBDB_LINKTEST_MAX; $i++) {
	    $selected = "";

	    if (strcmp($formfields["exp_linktest"], "$i") == 0)
		$selected = "selected";
	
	    echo "        <option $selected value=$i>Level $i - " .
		$linktest_levels[$i] . "</option>\n";
	}
	echo "       </select>";
	echo "    (<a href='$WIKIDOCURL/linktest'><b>What is this?</b></a>)";
	echo "    </td>
              </tr>\n";
    }

    echo "<tr>
              <td class='pad4' align=center colspan=2>
                <b><input type=submit name=swapin value='Instantiate'></b>
              </td>
         </tr>
        </form>
        </table>\n";
}

#
# On first load, display virgin form and exit.
#
if (!isset($swapin)) {
    $defaults = array();
    
    $defaults["eid"]		       = $template->tid();
    $defaults["description"]           = "";
    $defaults["preload"]               = "no";
    $defaults["savedisk"]              = "";
    $defaults["batched"]               = "";
    $defaults["exp_swappable"]         = "1";
    $defaults["parameter_xmlfile"]     = "";
    $defaults["exp_noswap_reason"]     = "";
    $defaults["exp_idleswap"]          = "1";
    $defaults["exp_linktest"]          = 3;    
    $defaults["exp_noidleswap_reason"] = "";
    $defaults["exp_idleswap_timeout"]  = TBGetSiteVar("idle/threshold");
    $defaults["exp_autoswap"]          = TBGetSiteVar("general/autoswap_mode");
    $defaults["exp_autoswap_timeout"]  = TBGetSiteVar("general/autoswap_threshold");

    #
    # Optional replay instance/run.
    #
    unset($replay_instance);

    if (isset($replay_instance_idx)) {
	if (!TBvalid_integer($replay_instance_idx)) {
	    PAGEARGERROR("Invalid characters in replay instance IDX!");
	}
    
	$replay_instance =
	    TemplateInstance::LookupByExptidx($replay_instance_idx);
	if (!$replay_instance) {
	    USERERROR("No such instance $replay_instance_idx in template!", 1);
	}
	$defaults["replay_instance_idx"] = $replay_instance_idx;

	if (isset($replay_run_idx)) {
	    if (!TBvalid_integer($replay_run_idx)) {
		PAGEARGERROR("Invalid characters in replay run IDX!");
	    }
	    $replay_instance->RunBindings($parameters);
	    $defaults["replay_run_idx"] = $replay_run_idx;
	}
	else {
	    $replay_instance->Bindings($parameters);	
	}
    }
    else {
        #
        # Look for formal parameters that the user can specify.
        #
	$template->FormalParameters($parameters);
    }
    
    #
    # Allow formfields that are already set to override defaults
    #
    if (isset($formfields)) {
	while (list ($field, $value) = each ($formfields)) {
	    $defaults[$field] = $formfields[$field];
	}
    }

    SPITFORM($template, $defaults, $parameters, 0);
    PAGEFOOTER();
    return;
}
elseif (! isset($formfields)) {
    PAGEARGERROR();
}

#
# Okay, validate form arguments.
#
$errors = array();
$eid    = "";

#
# EID:
#
if (!isset($formfields["eid"]) || $formfields["eid"] == "") {
    $errors["ID"] = "Missing Field";
}
elseif (!TBvalid_eid($formfields["eid"])) {
    $errors["ID"] = TBFieldErrorString();
}
elseif (Experiment::Lookup($pid, $formfields["eid"])) {
    $errors["ID"] = "Already in use";
}
else {
    $eid = $formfields["eid"];
}

# Set up command options
$command_options = "";

#
# Swappable
#
if (!isset($formfields["exp_swappable"]) ||
    strcmp($formfields["exp_swappable"], "1")) {
    $formfields["exp_swappable"] = 0;

    if (!isset($formfields["exp_noswap_reason"]) ||
        !strcmp($formfields["exp_noswap_reason"], "")) {

        if (! $isadmin) {
	    $errors["Not Swappable"] = "No justification provided";
        }
	else {
	    $formfields["exp_noswap_reason"] = "ADMIN";
        }
    }
    elseif (!TBvalid_description($formfields["exp_noswap_reason"])) {
	$errors["Not Swappable"] = TBFieldErrorString();
    }
    else {
	$command_options .=
	    " -S " . escapeshellarg($formfields["exp_noswap_reason"]);
    }
}
else {
    # For form redisplay.
    $formfields["exp_swappable"]     = 1;
    $formfields["exp_noswap_reason"] = "";
}

#
# Idle swap
#
if (!isset($formfields["exp_idleswap"]) ||
    strcmp($formfields["exp_idleswap"], "1")) {
    $formfields["exp_idleswap"] = 0;

    if (!isset($formfields["exp_noidleswap_reason"]) ||
	!strcmp($formfields["exp_noidleswap_reason"], "")) {
	if (! $isadmin) {
	    $errors["Not Idle-Swappable"] = "No justification provided";
	}
	else {
	    $formfields["exp_noidleswap_reason"] = "ADMIN";
	}
    }
    elseif (!TBvalid_description($formfields["exp_noidleswap_reason"])) {
	$errors["Not Idle-Swappable"] = TBFieldErrorString();
    }
    else {
	$command_options .= " -L " .
	    escapeshellarg($formfields["exp_noidleswap_reason"]);
    }
}
else {
    $formfields["exp_idleswap"]          = 1;
    $formfields["exp_noidleswap_reason"] = "";

    # Need this below;
    $idleswaptimeout = TBGetSiteVar("idle/threshold");

    # Proper idleswap timeout must be provided.
    if (!isset($formfields["exp_idleswap_timeout"]) ||
	!preg_match("/^[\d]+$/", $formfields["exp_idleswap_timeout"]) ||
	($formfields["exp_idleswap_timeout"] + 0) <= 0 ||
	($formfields["exp_idleswap_timeout"] + 0) > $idleswaptimeout) {
	$errors["Idleswap"] = "Invalid time provided - ".
	    "must be non-zero and less than $idleswaptimeout";
    }
    $command_options .= " -l " . (60 * $formfields["exp_idleswap_timeout"]);
}

#
# Autoswap.
#
if (!isset($formfields["exp_autoswap"]) ||
    strcmp($formfields["exp_autoswap"], "1")) {
    $formfields["exp_autoswap"] = 0;
}
else {
    $formfields["exp_autoswap"] = 1;
    
    if (!isset($formfields["exp_autoswap_timeout"]) ||
	!preg_match("/^[\d]+$/", $formfields["exp_idleswap_timeout"]) ||
	($formfields["exp_autoswap_timeout"] + 0) <= 0) {
	$errors["Max. Duration"] = "No or invalid time provided";
    }
    else {
	$command_options .= " -a " . (60*$formfields["exp_autoswap_timeout"]);
    }
}

#
# Save Disk
#
if (isset($formfields["exp_savedisk"]) &&
    strcmp($formfields["exp_savedisk"], "Yep") == 0) {
    $command_options .= " -s";
}

#
# Parameters. The XML file overrides stuff in the form. 
#
$template->FormalParameters($parameter_masterlist);
if (count($parameter_masterlist)) {
    if (isset($formfields["parameter_xmlfile"]) &&
	$formfields["parameter_xmlfile"] != "") {

	$parameter_xmlfile = $formfields["parameter_xmlfile"];
	    
	if (!preg_match("/^([-\@\w\.\/]+)$/", $parameter_xmlfile)) {
	    $errors["Parameter XML File"] =
		"Pathname includes illegal characters";
	}
	elseif (! VALIDUSERPATH($parameter_xmlfile)) {
	    $errors["Parameter XML File"] =
		"Must reside in one of: $TBVALIDDIRS";
	}
    	$deletexmlfile = 0;
    }
    else {
	#
	# Lets confirm that the user did not forget to set at least one value. 
	#
	$gotone = 0;
	while (list ($name, $default_value) = each ($parameter_masterlist)) {
	    if (isset($parameters[$name]) && $parameters[$name] != "") {
		$gotone = 1;
	    }
	}
	if (! $gotone) {
	    $errors["Parameters"] = "You did not set any values";
	}
	else {
  	    #
	    # Generate a temporary file and write in the XML goo.
	    #
	    list($usec, $sec) = explode(' ', microtime());
	    srand((float) $sec + ((float) $usec * 100000));
	    $foo = rand();

	    $parameter_xmlfile = "/tmp/$uid-$foo.xml";
	    $deletexmlfile = 1;

	    if (! ($fp = fopen($parameter_xmlfile, "w"))) {
		TBERROR("Could not create temp file $parameter_xmlfile", 1);
	    }
	
	    fwrite($fp, "<template_parameters>\n");

	    reset($parameter_masterlist);
	    while (list ($name,$default_value) = each($parameter_masterlist)) {
		if (isset($parameters[$name])) {
		    $value = $parameters[$name];
		}
		else {
		    $value = $default_value;
		}
	    
		fwrite($fp, " <parameter name=\"$name\">");
		fwrite($fp, "<value>$value</value></parameter>\n");
	    }
	    fwrite($fp, "</template_parameters>\n");
	    fclose($fp);
	    chmod($parameter_xmlfile, 0666);
	}
    }
    $command_options .= " -x $parameter_xmlfile";
}

#
# Description:
# 
if (isset($formfields["description"]) && $formfields["description"] != "") {
    if (!TBvalid_template_description($formfields["description"])) {
	$errors["Description"] = TBFieldErrorString();
    }
    else {
	$command_options .= " -E ". escapeshellarg($formfields["description"]);
    }
}

#
# Batchmode
#
if (isset($formfields["batched"]) && $formfields["batched"] == "Yep") {
    $command_options .= " -b";
    $batchmode = 1;
}

#
# Preload?
#
if (isset($formfields["preload"]) && $formfields["preload"] == "Yep") {
    $command_options .= " -p";
    $preload = 1;
}

#
# LinkTest
#
if (isset($formfields["exp_linktest"]) && $formfields["exp_linktest"] != "") {
    if (!preg_match("/^[\d]+$/", $formfields["exp_linktest"]) ||
	$formfields["exp_linktest"] < 0 || $formfields["exp_linktest"] > 4) {
	$errors["Linktest Option"] = "Invalid level selection";
    }
    else {
	$command_options .= " -t " . $formfields["exp_linktest"];
    }
}

#
# Replay info.
#
if (isset($formfields["replay_instance_idx"]) &&
    $formfields["replay_instance_idx"] != "") {
    if (!preg_match("/^[\d]+$/", $formfields["replay_instance_idx"])) {
	$errors["Replay Instance"] = "Invalid replay instance";
    }
    else {
	$command_options .= " -r " . $formfields["replay_instance_idx"];
    }
    if (isset($formfields["replay_run_idx"]) &&
	$formfields["replay_run_idx"] != "") {
	if (!preg_match("/^[\d]+$/", $formfields["replay_run_idx"])) {
	    $errors["Replay Run"] = "Invalid replay run";
	}
	else {
	    $command_options .= ":" . $formfields["replay_run_idx"];
	}
    }
}

if (count($errors)) {
    SPITFORM($template, $formfields, $parameters, $errors);
    PAGEFOOTER();
    exit(1);
}

#
# Avoid SIGPROF in child.
#
set_time_limit(0);

# Okay, we can spit back a header now that there is no worry of redirect.
PAGEHEADER("Instantiate Experiment Template");

echo $template->PageHeader();
echo "<font size=+2>, Instance <b>" .
      MakeLink("project", "pid=$pid", $pid) . "/" .
      MakeLink("experiment", "pid=$pid&eid=$eid", $eid) . 
     "</b></font>";
echo "<br><br>\n";

echo "<script type='text/javascript' language='javascript' ".
     "        src='template_sup.js'>\n";
echo "</script>\n";

STARTBUSY("Starting template instantiation!");

#
# Run the backend script.
#
$retval = SUEXEC($uid, "$unix_pid,$unix_gid",
		 "webtemplate_instantiate ".
		    "$command_options -e $eid $guid/$vers",
		 SUEXEC_ACTION_IGNORE);

HIDEBUSY();

if ($retval) {
    #
    # Fatal Error. Report to the user, even though there is not much he can
    # do with the error. Also reports to tbops.
    # 
    if ($retval < 0) {
	SUEXECERROR(SUEXEC_ACTION_CONTINUE);
    }

    # User error. Tell user and exit.
    SUEXECERROR(SUEXEC_ACTION_USERERROR);
    return;
}

#
# We need to locate this instance for STARTLOG() below.
#
if (!preg_match("/^Instance\s+[-\w]+\/[-\w]+\s+\((\d*)\)/",
		$suexec_output_array[count($suexec_output_array)-1],
		$matches)) {
    TBERROR("Could not locate instance object for $pid/$eid", 1);
}
$instance = TemplateInstance::LookupByIdx($matches[1]);
if (!$instance) {
    TBERROR("Could not map instance idx " . $matches[1] . " to its object!",1);
}

#
# This does both the log output, and the state change watcher popup
#
if ($batchmode && !$preload) {
    echo "You template instantation has been queued and will run when
          enough resources become available. This might happen
          immediately, or it may take hours or days; you will be
          notified via email when insantiation is complete, and again when
          your experiment has completed.\n";
}
STARTLOG($instance->GetLogfile());

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
