<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
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
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

# This will not return if its a sajax request.
include("showlogfile_sup.php3");

# Used below
unset($parameter_xmlfile);
$deletexmlfile = 0;

#
# Run the script backend
#
function DOIT($instance, $action, $command_options)
{
    global $guid, $version, $pid, $gid, $eid, $uid;
    global $deletexmlfile, $parameter_xmlfile;
    $message = "";

    $command_options = "-e $eid " . $command_options;
    
    if ($action == "start") {
	PAGEHEADER("Start new Experiment Run");
	$message = "Starting new experiment run";
	$command_options = "-a start " . $command_options;
    }
    elseif ($action == "abort") {
	PAGEHEADER("Abort Experiment Run");
	$message = "Aborting experiment run";
	$command_options = "-a abort " . $command_options;
    }
    else {
	PAGEHEADER("Stop current Experiment Run");
	$message = "Stopping current experiment run";
	$command_options = "-a stop " . $command_options;
    }	

    #
    # Grab the unix GID for running scripts.
    #
    TBGroupUnixInfo($pid, $gid, $unix_gid, $unix_name);

    #
    # Avoid SIGPROF in child.
    #
    set_time_limit(0);

    echo "<font size=+2>Template <b>" .
            MakeLink("template",
		     "guid=$guid&version=$version", "$guid/$version") .
            "</b>, Instance <b>" .
            MakeLink("project", "pid=$pid", $pid) . "/" .
            MakeLink("experiment", "pid=$pid&eid=$eid", $eid);
    echo "</b></font>\n";
    echo "<br><br>\n";

    echo "<script type='text/javascript' language='javascript' ".
	 "        src='template_sup.js'>\n";
    echo "</script>\n";

    STARTBUSY($message);

    #
    # Run the backend script.
    #
    $retval = SUEXEC($uid, "$pid,$unix_gid",
		     "webtemplate_exprun $command_options $guid/$version",
		     SUEXEC_ACTION_IGNORE);

    CLEARBUSY();

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

    STARTLOG($pid, $eid);
}

#
# Run the script backend
#
function DOTIME($instance, $action)
{
    global $guid, $version, $pid, $gid, $eid, $uid;
    $message = "";

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
    # Grab the unix GID for running scripts.
    #
    TBGroupUnixInfo($pid, $gid, $unix_gid, $unix_name);

    #
    # Avoid SIGPROF in child.
    #
    set_time_limit(0);

    echo "<font size=+2>Template <b>" .
            MakeLink("template",
		     "guid=$guid&version=$version", "$guid/$version") .
            "</b>, Instance <b>" .
            MakeLink("project", "pid=$pid", $pid) . "/" .
            MakeLink("experiment", "pid=$pid&eid=$eid", $eid);
    echo "</b></font>\n";
    echo "<br><br>\n";

    echo "<script type='text/javascript' language='javascript' ".
	 "        src='template_sup.js'>\n";
    echo "</script>\n";

    STARTBUSY($message);

    #
    # Run the backend script.
    #
    $retval = SUEXEC($uid, "$pid,$unix_gid",
		     "webtemplate_exprun $command_options $guid/$version",
		     SUEXEC_ACTION_IGNORE);

    CLEARBUSY();

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

    PAGEREPLACE("showexp.php3?pid=$pid&eid=$eid");
}

#
# Spit the form out using the array of data.
#
function SPITFORM($instance, $formfields, $parameters, $errors)
{
    global $TBDB_EIDLEN;
    global $guid, $version, $pid, $eid;

    PAGEHEADER("Start new Experiment Run");

    echo "<font size=+2>Template <b>" .
            MakeLink("template",
		     "guid=$guid&version=$version", "$guid/$version") .
            "</b>, Instance <b>" .
            MakeLink("project", "pid=$pid", $pid) . "/" .
            MakeLink("experiment", "pid=$pid&eid=$eid", $eid);
    echo "</b></font>\n";
    echo "<br><br>\n";

    echo "<center>\n";
    $template = $instance->template();
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
    

    echo "<form action=template_exprun.php".
	    "?action=start&guid=$guid&version=$version&eid=$eid ".
	 "      method=post>\n";
    echo "<table align=center border=1>\n";
    
    #
    # RunID:
    #
    echo "<tr>
              <td class='pad4'>ID:
              <br><font size='-1'>(alphanumeric, no blanks)</font></td>
              <td class='pad4' class=left>
                  <input type=text
                         name=\"formfields[runid]\"
                         value=\"" . $formfields[runid] . "\"
	                 size=$TBDB_EIDLEN
                         maxlength=$TBDB_EIDLEN>
              &nbsp (optional; we will make up one for you)
              </td>
          </tr>\n";

    #
    # Clean logs before starting run?
    #
    echo "<tr>
	      <td class='pad4'>Clean Logs:</td>
              <td class='pad4' class=left>
  	          <input type=checkbox name='formfields[clean]' value='Yep'";

    if (isset($formfields[clean]) &&
	strcmp($formfields[clean], "Yep") == 0) {
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

    if (isset($formfields[swapmod]) &&
	strcmp($formfields[swapmod], "Yep") == 0) {
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
	            ereg_replace("\r", "", $formfields[description]) .
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

	    echo "<tr>
                    <td class='pad4'>$name</td>
                    <td class='pad4' class=left>
                        <input type=text
                               id='parameter_$name'
                               name=\"parameters[$name]\"
                               value=\"\"
	                       size=60
                               maxlength=1024>
                    </td>
                  </tr>\n";
	}
	echo "<tr><td>&nbsp;&nbsp;<b>or</b> XML file:
               </td><td></td></tr>\n";
	echo "<tr>
                  <td class='pad4'>On Server<br>
                           <font size='-1'>(<code>/proj</code>,
                      <code>/groups</code>, <code>/users</code>)</font></td>
                  <td class='pad4'>
	              <input type=text
                             name=\"formfields[parameter_xmlfile]\"
                             value=\"" . $formfields[parameter_xmlfile] . "\"
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
if (!isset($eid) ||
    strcmp($eid, "") == 0) {
    USERERROR("You must provide a template instance ID", 1);
}
if (!TBvalid_guid($guid)) {
    PAGEARGERROR("Invalid characters in GUID!");
}
if (!TBvalid_integer($version)) {
    PAGEARGERROR("Invalid characters in version!");
}
if (!TBvalid_eid($eid)) {
    PAGEARGERROR("Invalid characters in instance ID!");
}

#
# Check to make sure this is a valid template and user has permission.
#
$template = Template::Lookup($guid, $version);
if (!$template) {
    USERERROR("The experiment template $guid/$version is not a valid ".
              "experiment template!", 1);
}
if (! $template->AccessCheck($uid, $TB_EXPT_MODIFY)) {
    USERERROR("You do not have permission to modify experiment template ".
	      "$guid/$version!", 1);
}

# Need these below.
$pid = $template->pid();
$gid = $template->gid();

if (($exptidx = TBExptIndex($pid, $eid)) < 0) {
    TBERROR("No such experiment '$eid' for template $guid/$version/$eid", 1);
}

#
# Check to make sure and a valid instance that is actually swapped in.
#
$instance = TemplateInstance::LookupByExptidx($exptidx);
if (!$instance) {
    TBERROR("Template Instance $guid/$version/$exptidx is not ".
	    "a valid experiment template instance!", 1);
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
elseif (!isset($exprun)) {
    #
    # On first load, display virgin form and exit.
    #
    $defaults['runid'] = $instance->NextRunID();

    #
    # Get the current bindings for the template instance.
    #
    $instance->Bindings($bindings);
    
    #
    # Allow formfields that are already set to override defaults
    #
    if (isset($formfields)) {
	while (list ($field, $value) = each ($formfields)) {
	    $defaults[$field] = $formfields[$field];
	}
    }

    SPITFORM($instance, $defaults, $bindings, 0);
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

# Set up command options
$command_options = " ";

#
# RunID:
#
if (!isset($formfields[runid]) || $formfields[runid] == "") {
    $errors["ID"] = "Missing Field";
}
elseif (!TBvalid_eid($formfields[runid])) {
    $errors["ID"] = TBFieldErrorString();
}
elseif (TBValidExperiment($pid, $formfields[runid])) {
    $errors["ID"] = "Already in use";
}
else {
    $command_options .= " -r " . escapeshellarg($formfields[runid]);
}

#
# Clean?
#
if (isset($formfields[clean]) && $formfields[clean] == "Yep") {
    $command_options .= " -c";
}

#
# Swapmod?
#
if (isset($formfields[swapmod]) && $formfields[swapmod] == "Yep") {
    $command_options .= " -m";
}

#
# Description:
# 
if (isset($formfields[description]) && $formfields[description] != "") {
    if (!TBvalid_template_description($formfields[description])) {
	$errors["Description"] = TBFieldErrorString();
    }
    else {
	$command_options .= " -E " . escapeshellarg($formfields[description]);
    }
}

#
# Parameters. The XML file overrides stuff in the form. 
#
$template->FormalParameters($parameter_masterlist);
if (count($parameter_masterlist)) {
    if (isset($formfields[parameter_xmlfile]) &&
	$formfields[parameter_xmlfile] != "") {

	$parameter_xmlfile = $formfields[parameter_xmlfile];
	    
	if (!preg_match("/^([-\@\w\.\/]+)$/", $parameter_xmlfile)) {
	    $errors["Parameter XML File"] =
		"Pathname includes illegal characters";
	}
	elseif (! ereg("^$TBPROJ_DIR/.*",  $parameter_xmlfile) &&
		! ereg("^$TBUSER_DIR/.*",  $parameter_xmlfile) &&
		! ereg("^$TBGROUP_DIR/.*", $parameter_xmlfile)) {
	    $errors["Parameter XML File"] = "Must reside in either ".
		"$TBUSER_DIR/, $TBPROJ_DIR/, or $TBGROUP_DIR/";
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
    SPITFORM($instance, $formfields, $parameters, $errors);
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
