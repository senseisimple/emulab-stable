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

# This will not return if its a sajax request.
include("showlogfile_sup.php3");

# Used below
unset($parameter_xmlfile);
$deletexmlfile = 0;

#
# Verify page arguments.
#
$reqargs  = RequiredPageArguments("instance",   PAGEARG_INSTANCE);
$optargs  = OptionalPageArguments("action",     PAGEARG_STRING,
				  "exprun",     PAGEARG_STRING,
				  "parameters", PAGEARG_ARRAY,
				  "formfields", PAGEARG_ARRAY);
$template = $instance->GetTemplate();

# Need these below.
$guid = $template->guid();
$vers = $template->vers();
$pid  = $template->pid();
$eid  = $instance->eid();
$unix_gid   = $template->UnixGID();
$exptidx    = $instance->exptidx();
$experiment = $instance->GetExperiment();

if (! $template->AccessCheck($this_user, $TB_EXPT_MODIFY)) {
    USERERROR("You do not have permission to export in template ".
	      "$guid/$vers!", 1);
}

#
# Run the script backend
#
function DOIT($instance, $action, $command_options)
{
    global $guid, $vers, $pid, $unix_gid, $eid, $uid;
    global $deletexmlfile, $parameter_xmlfile;
    $message    = "";
    $template   = $instance->GetTemplate();
    $experiment = $instance->GetExperiment();

    $command_options = "-e $eid " . $command_options;
    
    if ($action == "start") {
	PAGEHEADER("Start new Run");
	$message = "Starting new experiment run";
	$command_options = "-a start " . $command_options;
    }
    elseif ($action == "abort") {
	PAGEHEADER("Abort Run");
	$message = "Aborting experiment run";
	$command_options = "-a abort " . $command_options;
    }
    elseif ($action == "modify") {
	PAGEHEADER("Modify Run Resources");
	$message = "Modifying resources for run";
	$command_options = "-a modify " . $command_options;
    }
    else {
	PAGEHEADER("Stop current Run");
	$message = "Stopping current run";
	$command_options = "-a stop " . $command_options;
    }	

    #
    # Avoid SIGPROF in child.
    #
    set_time_limit(0);

    echo $instance->ExpPageHeader();
    echo "<br><br>\n";

    echo "<script type='text/javascript' language='javascript' ".
	 "        src='template_sup.js'>\n";
    echo "</script>\n";

    STARTBUSY($message);

    #
    # Run the backend script.
    #
    $retval = SUEXEC($uid, "$pid,$unix_gid",
		     "webtemplate_exprun $command_options $guid/$vers",
		     SUEXEC_ACTION_IGNORE);

    HIDEBUSY();

    if ($deletexmlfile) {
	unlink($parameter_xmlfile);
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

    STARTLOG($experiment);
}

#
# Run the script backend
#
function DOTIME($instance, $action)
{
    global $guid, $vers, $pid, $unix_gid, $eid, $uid;
    $message    = "";
    $template   = $instance->GetTemplate();
    $experiment = $instance->GetExperiment();

    if ($action == "pause") {
	PAGEHEADER("Pause Experiment Time");
	$message = "Pausing experiment runtime";
    }
    else {
	PAGEHEADER("Continue Experiment Time");
	$message = "Continuing experiment runtime";
    }	

    $command_options = "-e $eid -a $action ";
    
    #
    # Avoid SIGPROF in child.
    #
    set_time_limit(0);

    echo $instance->ExpPageHeader();
    echo "<br><br>\n";

    echo "<script type='text/javascript' language='javascript' ".
	 "        src='template_sup.js'>\n";
    echo "</script>\n";

    STARTBUSY($message);

    #
    # Run the backend script.
    #
    $retval = SUEXEC($uid, "$pid,$unix_gid",
		     "webtemplate_exprun $command_options $guid/$vers",
		     SUEXEC_ACTION_IGNORE);

    HIDEBUSY();

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

    PAGEREPLACE(CreateURL("showexp", $experiment));
}

#
# Spit the form out using the array of data.
#
function SPITFORM($action, $instance, $formfields, $parameters, $errors)
{
    global $TBDB_EIDLEN;
    global $TBVALIDDIRS_HTML;
    $template   = $instance->GetTemplate();

    if ($action == "modify") {
	PAGEHEADER("Modify Resources");
    }
    else {
	PAGEHEADER("Start New Run");
    }

    echo $instance->ExpPageHeader();
    echo "<br><br>\n";

    echo "<center>\n";
    $template->Show();
    echo "</center>\n";
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

    #
    # Get the default params for the template, instance and the previous run.
    # These go out for the user to select via a button, which will provide an
    # initial setting for the parameters.
    #
    $template->FormalParameters($formal_parameters);
    $template->FormalParameterMouseOvers($mouseovers);
    $instance->Bindings($instance_parameters);
    $instance->RunBindings($instance->LastRunIdx(), $lastrun_parameters);

    #
    # Spit out some inline script. Its not as if there will be hundreds
    # of parameters, right?
    #
    echo "<script type='text/javascript' language='javascript' ".
	 "        src='template_sup.js'>\n";
    echo "</script>\n";
    echo "<script language=JavaScript>\n";
    echo "var formal_names  = new Array();\n";
    echo "var template_values = new Array();\n";
    echo "var instance_values = new Array();\n";
    echo "var lastrun_values  = new Array();\n";
    $i = 0;
    while (list ($name, $value) = each ($formal_parameters)) {
	echo "formal_names[$i] = '$name';\n";
	echo "template_values[$i] = '$value';\n";
	$i++;
    }
    $i = 0;
    while (list ($name, $value) = each ($instance_parameters)) {
	echo "instance_values[$i] = '$value';\n";
	$i++;
    }
    $i = 0;
    while (list ($name, $value) = each ($lastrun_parameters)) {
	echo "lastrun_values[$i] = '$value';\n";
	$i++;
    }
    echo "</script>\n";

    $url = CreateURL("template_exprun", $instance);

    echo "<form action='${url}&action=$action' method=post>\n";
    echo "<table align=center border=1>\n";
    
    #
    # RunID:
    #
    if ($action == "start" ||
	($action == "modify" && !$instance->runidx())) {
	
	echo "<tr>
                  <td class='pad4'>ID:
                  <br><font size='-1'>(alphanumeric, no blanks)</font></td>
                  <td class='pad4' class=left>";
	echo "    <input type=text
                         name=\"formfields[runid]\"
                         value=\"" . $formfields["runid"] . "\"
	                 size=$TBDB_EIDLEN
                         maxlength=$TBDB_EIDLEN>\n";
	echo "    </td>
              </tr>\n";
    }

    #
    # Clean logs before starting run?
    #
    echo "<tr>
	      <td class='pad4'>Clean Logs:</td>
              <td class='pad4' class=left>
  	          <input type=checkbox name='formfields[clean]' value='Yep'";

    if (isset($formfields["clean"]) &&
	strcmp($formfields["clean"], "Yep") == 0) {
	echo " checked='1'";
    }
    echo ">";
    echo "&nbsp (run '<tt>loghole clean</tt>' before starting run)
	    </td>
	  </tr>\n";

    #
    # Swapmod?
    #
    echo "<tr>
	      <td class='pad4'>Reparse NS file?:</td>
              <td class='pad4' class=left>
  	          <input type=checkbox name='formfields[swapmod]' value='Yep'";

    if (isset($formfields["swapmod"]) &&
	strcmp($formfields["swapmod"], "Yep") == 0) {
	echo " checked='1'";
    }
    echo ">";
    echo "&nbsp (effectively a '<tt>swap modify</tt>')
	    </td>
	  </tr>\n";

    echo "<tr>
              <td colspan=2>
               Use this text area for an (optional) description:
              </td>
          </tr>
          <tr>
              <td colspan=2 align=center class=left>
                  <textarea name=\"formfields[description]\"
                    rows=4 cols=80>" .
	            ereg_replace("\r", "", $formfields["description"]) .
	           "</textarea>
              </td>
          </tr>\n";

    if (count($parameters)) {
        #
	# Table of inputs.
	#
	echo "<tr>
		  <td class='pad4'>Formal Parameters:</td>
		  <td>
 		    <table cellpadding=0 cellspacing=0 border=0>\n";

	echo "<tr><td>Choose Values:</td><td>";
	echo "<table cellpadding=0 cellspacing=0 border=0>\n";
	echo "<tr><td>\n";
	echo " <button name=formals type=button value=Formals ";
	echo "  onclick=\"SetRunParams(formal_names, template_values);\">";
	echo "Template</button>\n";
	echo "</td><td>\n";
	echo " <button name=instance type=button value=Instance ";
	echo "  onclick=\"SetRunParams(formal_names, instance_values);\">";
	echo "Instance</button>\n";
	echo "</td><td>\n";
	echo " <button name=lastrun type=button value='Previous Run' ";
	echo "  onclick=\"SetRunParams(formal_names, lastrun_values);\">";
	echo "Previous Run</button>\n";
	echo "</tr></table>\n";
	
	while (list ($name, $value) = each ($parameters)) {
	    if (!isset($value))
		$value = "&nbsp";
	    $mouseover = (isset($mouseovers[$name]) ? $mouseovers[$name] : "");

	    echo "<tr>
                    <td class='pad4' $mouseover>$name</td>
                    <td class='pad4' class=left>
                        <input type=text
                               id='parameter_$name'
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

    echo "<tr>
              <td class='pad4' align=center colspan=2>
                <b><input type=submit name=exprun value='Start'></b>
              </td>
         </tr>
        </form>
        </table>\n";
}

if (isset($action) && ($action == "stop" || $action == "abort")) {
    # Run the backend script.
    DOIT($instance, $action, "");
    PAGEFOOTER();
    return;
}
elseif (isset($action) && ($action == "pause" || $action == "continue")) {
    # Run the backend script.
    DOTIME($instance, $action, "");
    PAGEFOOTER();
    return;
}
elseif (! isset($formfields)) {
    $defaults = array();
    
    #
    # On first load, display virgin form and exit.
    #
    $defaults['runid']   = $instance->NextRunID();
    $defaults['clean']   = "";
    $defaults['swapmod'] = "";
    $defaults['description'] = "";
    $defaults['parameter_xmlfile'] = "";

    #
    # Get the current bindings of the current run.
    #
    $instance->RunBindings($instance->LastRunIdx(), $bindings);
    
    #
    # Allow formfields that are already set to override defaults
    #
    if (isset($formfields)) {
	while (list ($field, $value) = each ($formfields)) {
	    $defaults[$field] = $formfields[$field];
	}
    }

    SPITFORM($action, $instance, $defaults, $bindings, 0);
    PAGEFOOTER();
    return;
}

#
# Okay, validate form arguments.
#
$errors = array();

# Set up command options
$command_options = " ";

#
# RunID:
#
if ($action == "start" ||
    ($action == "modify" && !$instance->runidx())) {
    if (!isset($formfields["runid"]) || $formfields["runid"] == "") {
	$errors["ID"] = "Missing Field";
    }
    elseif (!TBvalid_eid($formfields["runid"])) {
	$errors["ID"] = TBFieldErrorString();
    }
    elseif (! $instance->UniqueRunID($formfields["runid"])) {
	$errors["ID"] = "Already in use";
    }
    else {
	$command_options .= " -r " . escapeshellarg($formfields["runid"]);
    }
}

#
# Clean?
#
if (isset($formfields["clean"]) && $formfields["clean"] == "Yep") {
    $command_options .= " -c";
}

#
# Swapmod?
#
if (isset($formfields["swapmod"]) && $formfields["swapmod"] == "Yep") {
    $command_options .= " -m";
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
	    while (list($name,$default_value) = each ($parameter_masterlist)) {
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

if (count($errors)) {
    SPITFORM($action, $instance, $formfields, $parameters, $errors);
    PAGEFOOTER();
    exit(1);
}

# Run the backend script.
DOIT($instance, $action, $command_options);

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
