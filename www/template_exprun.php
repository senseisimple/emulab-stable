<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("template_defs.php");

#
# No PAGEHEADER since we spit out a Location header later. See below.
#

#
# Only known and logged in users.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

# Used below
unset($parameter_xmlfile);
$deletexmlfile = 0;

#
# Spit the form out using the array of data.
#
function SPITFORM($formfields, $parameters, $errors)
{
    global $TBDB_EIDLEN;
    global $guid, $version, $exptidx;

    PAGEHEADER("Start an Experiment Run");

    echo "<center>\n";
    SHOWTEMPLATE($guid, $version);
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

    echo "<form action=template_exprun.php".
	    "?action=start&guid=$guid&version=$version&exptidx=$exptidx ".
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

    echo "<tr>
              <td colspan=2>
               Use this text area to (optionally) describe your experiment:
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
	
	while (list ($name, $value) = each ($parameters)) {
	    if (!isset($value))
		$value = "&nbsp";

	    echo "<tr>
                    <td class='pad4'>$name</td>
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
if (!isset($exptidx) ||
    strcmp($exptidx, "") == 0) {
    USERERROR("You must provide a template instance ID", 1);
}
if (!TBvalid_guid($guid)) {
    PAGEARGERROR("Invalid characters in GUID!");
}
if (!TBvalid_integer($version)) {
    PAGEARGERROR("Invalid characters in version!");
}
if (!TBvalid_integer($exptidx)) {
    PAGEARGERROR("Invalid characters in instance ID!");
}

#
# Check to make sure this is a valid template.
#
if (! TBValidExperimentTemplate($guid, $version)) {
    USERERROR("The experiment template $guid/$version is not a valid ".
              "experiment template!", 1);
}

#
# Check to make sure and a valid instance that is actually swapped in.
#
if (! TBValidExperimentTemplateInstance($guid, $version, $exptidx)) {
    USERERROR("Experiment Template Instance $guid/$version/$exptidx is not ".
              "a valid experiment template instance!", 1);
}

#
# Verify Permission.
#
if (! TBExptTemplateAccessCheck($uid, $guid, $TB_EXPT_UPDATE)) {
    USERERROR("You do not have permission to instantiate experiment template ".
	      "$guid/$version!", 1);
}

#
# On first load, display virgin form and exit.
#
if (!isset($exprun)) {
    #
    # 
    #
    TBTemplateNextExperimentRun($guid, $version, $exptidx, $nextidx);
    
    $defaults['runid'] = "T${nextidx}";

    #
    # Get the current bindings for the template instance.
    #
    TBTemplateInstanceBindings($guid, $version, $exptidx, $bindings);
    
    #
    # Allow formfields that are already set to override defaults
    #
    if (isset($formfields)) {
	while (list ($field, $value) = each ($formfields)) {
	    $defaults[$field] = $formfields[$field];
	}
    }

    SPITFORM($defaults, $bindings, 0);
    PAGEFOOTER();
    return;
}
elseif (! isset($formfields)) {
    PAGEARGERROR();
}

# Need this below.
if (! TBGuid2PidGid($guid, $pid, $gid)) {
    TBERROR("Could not get template pid,gid for template $guid", 1);
}

#
# Okay, validate form arguments.
#
$errors = array();
$runid  = "";

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
    $runid = $formfields[runid];
}

# Set up command options
$command_options = " -a start ";

#
# Description:
# 
if (!isset($formfields[description]) || $formfields[description] == "") {
    $errors["Description"] = "Missing Field";
}
elseif (!TBvalid_template_description($formfields[description])) {
    $errors["Description"] = TBFieldErrorString();
}
else {
    $command_options .= " -E " . escapeshellarg($formfields[description]);
}

#
# Parameters. The XML file overrides stuff in the form. 
#
TBTemplateFormalParameters($guid, $version, $parameter_masterlist);
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
	# Generate a temporary file and write in the XML goo.
	#
	list($usec, $sec) = explode(' ', microtime());
	srand((float) $sec + ((float) $usec * 100000));
	$foo = rand();

	$parameter_xmlfile = "/tmp/$uid-$foo.xml";
    	$deletexmlfile = 1;

	if (! ($fp = fopen($parameter_xmlfile, "w"))) {
	    TBERROR("Could not create temporary file $parameter_xmlfile", 1);
	}
	
	fwrite($fp, "<template_parameters>\n");

	while (list ($name, $default_value) = each ($parameter_masterlist)) {
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
    $command_options .= " -p $parameter_xmlfile";
}

if (count($errors)) {
    SPITFORM($formfields, $parameters, $errors);
    PAGEFOOTER();
    exit(1);
}

#
# Grab the unix GID for running scripts.
#
if (! TBGuid2PidGid($guid, $pid, $gid)) {
    TBERROR("Could not get template pid,gid for template $guid", 1);
}
TBGroupUnixInfo($pid, $gid, $unix_gid, $unix_name);

#
# Avoid SIGPROF in child.
#
set_time_limit(0);

# Okay, we can spit back a header now that there is no worry of redirect.
PAGEHEADER("Start Experiment Template");

echo "<font size=+2>Experiment Template <b>" .
        MakeLink("template",
		 "guid=$guid&version=$version", "$guid/$version") . 
      "</b></font>\n";
echo "<br><br>\n";

echo "<b>Starting Experiment run</b> ... ";
echo "<br><br>\n";
flush();

#
# Run the backend script.
#
$retval = SUEXEC($uid, "$pid,$unix_gid",
		 "webtemplate_exprun $command_options $guid/$version $exptidx",
		 SUEXEC_ACTION_IGNORE);

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

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
