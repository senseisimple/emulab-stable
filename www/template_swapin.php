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

# This will not return if its a sajax request.
include("showlogfile_sup.php3");

# Used below
unset($parameter_xmlfile);
$deletexmlfile = 0;
$batchmode     = 0;

#
# Spit the form out using the array of data.
#
function SPITFORM($formfields, $parameters, $errors)
{
    global $TBDB_EIDLEN;
    global $guid, $version;

    PAGEHEADER("Instantiate an Experiment Template");

    echo "<center>\n";
    SHOWTEMPLATE($guid, $version);
    echo "</center>\n";
    echo "<br>\n";

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

    echo "<form action=template_swapin.php?guid=$guid&version=$version ".
	 "      method=post>\n";
    echo "<table align=center border=1>\n";

    #
    # EID:
    #
    echo "<tr>
              <td class='pad4'>ID:
              <br><font size='-1'>(alphanumeric, no blanks)</font></td>
              <td class='pad4' class=left>
                  <input type=text
                         name=\"formfields[eid]\"
                         value=\"" . $formfields[eid] . "\"
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
    echo htmlspecialchars($formfields[exp_noswap_reason], ENT_QUOTES);
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
		    <a href='$TBDOCBASE/docwrapper.php3?".
	                 "docname=swapping.html#swapping'>
		    Swapping:</td>
		  <td>
		  <table cellpadding=0 cellspacing=0 border=0><tr>
		  <td><input type='checkbox'
			 name='formfields[exp_idleswap]'
			 value='1'";
	if ($formfields[exp_idleswap] == "1") {
	    echo " checked='1'";
	}
	echo "></td>
		  <td><a href='$TBDOCBASE/docwrapper.php3?".
	                    "docname=swapping.html#idleswap'>
		  <b>Idle-Swap:</b></a> Swap out this experiment
		  after 
		  <input type='text' name='formfields[exp_idleswap_timeout]'
			 value='";
	echo htmlspecialchars($formfields[exp_idleswap_timeout], ENT_QUOTES);
	echo "' size='3'> hours idle.</td>
		  </tr><tr>
		  <td> </td>
		  <td>If not, why not?<br><textarea rows=2 cols=50
			      name='formfields[exp_noidleswap_reason]'>";
			      
	echo htmlspecialchars($formfields[exp_noidleswap_reason], ENT_QUOTES);
	echo "</textarea></td>
		  </tr><tr>
		  <td><input type='checkbox'
			 name='formfields[exp_autoswap]'
			 value='1' ";
	if ($formfields[exp_autoswap] == "1") {
	    echo " checked='1'";
	}
	echo "></td>
		  <td><a href='$TBDOCBASE/docwrapper.php3?".
	                    "docname=swapping.html#autoswap'>
		  <b>Max. Duration:</b></a> Swap out after
		  <input type='text' name='formfields[exp_autoswap_timeout]'
			 value='";
	echo htmlspecialchars($formfields[exp_autoswap_timeout], ENT_QUOTES);
	echo "' size='3'> hours, even if not idle.</td>
		  </tr>";

	if (STUDLY() || $EXPOSESTATESAVE) {
	    echo "<tr><td>
	         <input type=checkbox name='formfields[exp_savedisk]'
	         value='Yep'";

	    if (isset($formfields[exp_savedisk]) &&
		strcmp($formfields[exp_savedisk], "Yep") == 0) {
		    echo " checked='1'";
	    }

	    echo "></td>\n";
	    echo "<td><a href='$TBDOCBASE/docwrapper.php3?".
		              "docname=swapping.html#swapstatesave'>
		  <b>State Saving:</b></a> Save disk state on swapout</td>
		  </tr>";
	
	}
	echo "</table></td></tr>";
    }
    
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

    #
    # Batch Experiment?
    #
    echo "<tr>
	      <td class='pad4' colspan=2>
	      <input type=checkbox name='formfields[batched]' value='Yep'";

    if (isset($formfields[batched]) &&
	strcmp($formfields[batched], "Yep") == 0) {
	echo " checked='1'";
    }
    echo ">\n";
    echo "Batch Mode Instantiation &nbsp;
	  <font size='-1'>(See
          <a href='$TBDOCBASE/tutorial/tutorial.php3#BatchMode'>Tutorial</a>
          for more information)</font>
	  </td>
	  </tr>\n";

    echo "<tr>
              <td class='pad4' align=center colspan=2>
                <b><input type=submit name=swapin value='Instantiate'></b>
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
if (!TBvalid_guid($guid)) {
    PAGEARGERROR("Invalid characters in GUID!");
}
if (!TBvalid_integer($version)) {
    PAGEARGERROR("Invalid characters in version!");
}

#
# Check to make sure this is a valid template.
#
if (! TBValidExperimentTemplate($guid, $version)) {
    USERERROR("The experiment template $guid/$version is not a valid ".
              "experiment template!", 1);
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
if (!isset($swapin)) {
    TBTemplateTid($guid, $version, $tid);
    
    $defaults[eid]		     = $tid;
    $defaults[exp_swappable]         = "1";
    $defaults[exp_noswap_reason]     = "";
    $defaults[exp_idleswap]          = "1";
    $defaults[exp_noidleswap_reason] = "";
    $defaults[exp_idleswap_timeout]  = TBGetSiteVar("idle/threshold");
    $defaults[exp_autoswap]          = TBGetSiteVar("general/autoswap_mode");
    $defaults[exp_autoswap_timeout]  = TBGetSiteVar("general/autoswap_threshold");
    #
    # Look for formal parameters that the user can specify.
    #
    TBTemplateFormalParameters($guid, $version, $parameters);
    
    #
    # Allow formfields that are already set to override defaults
    #
    if (isset($formfields)) {
	while (list ($field, $value) = each ($formfields)) {
	    $defaults[$field] = $formfields[$field];
	}
    }

    SPITFORM($defaults, $parameters, 0);
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
$eid    = "";

#
# EID:
#
if (!isset($formfields[eid]) || $formfields[eid] == "") {
    $errors["ID"] = "Missing Field";
}
elseif (!TBvalid_eid($formfields[eid])) {
    $errors["ID"] = TBFieldErrorString();
}
elseif (TBValidExperiment($pid, $formfields[eid])) {
    $errors["ID"] = "Already in use";
}
else {
    $eid = $formfields[eid];
}

# Set up command options
$command_options = "";

#
# Swappable
#
if (!isset($formfields[exp_swappable]) ||
    strcmp($formfields[exp_swappable], "1")) {
    $formfields[exp_swappable] = 0;

    if (!isset($formfields[exp_noswap_reason]) ||
        !strcmp($formfields[exp_noswap_reason], "")) {

        if (! ISADMIN($uid)) {
	    $errors["Not Swappable"] = "No justification provided";
        }
	else {
	    $formfields[exp_noswap_reason] = "ADMIN";
        }
    }
    elseif (!TBvalid_description($formfields[exp_noswap_reason])) {
	$errors["Not Swappable"] = TBFieldErrorString();
    }
    else {
	$command_options .=
	    " -S " . escapeshellarg($formfields[exp_noswap_reason]);
    }
}
else {
    # For form redisplay.
    $formfields[exp_swappable]     = 1;
    $formfields[exp_noswap_reason] = "";
}

#
# Idle swap
#
if (!isset($formfields[exp_idleswap]) ||
    strcmp($formfields[exp_idleswap], "1")) {
    $formfields[exp_idleswap] = 0;

    if (!isset($formfields[exp_noidleswap_reason]) ||
	!strcmp($formfields[exp_noidleswap_reason], "")) {
	if (! ISADMIN($uid)) {
	    $errors["Not Idle-Swappable"] = "No justification provided";
	}
	else {
	    $formfields[exp_noidleswap_reason] = "ADMIN";
	}
    }
    elseif (!TBvalid_description($formfields[exp_noidleswap_reason])) {
	$errors["Not Idle-Swappable"] = TBFieldErrorString();
    }
    else {
	$command_options .= " -L " .
	    escapeshellarg($formfields[exp_noidleswap_reason]);
    }
}
else {
    $formfields[exp_idleswap]          = 1;
    $formfields[exp_noidleswap_reason] = "";

    # Need this below;
    $idleswaptimeout = TBGetSiteVar("idle/threshold");

    # Proper idleswap timeout must be provided.
    if (!isset($formfields[exp_idleswap_timeout]) ||
	!preg_match("/^[\d]+$/", $formfields[exp_idleswap_timeout]) ||
	($formfields[exp_idleswap_timeout] + 0) <= 0 ||
	($formfields[exp_idleswap_timeout] + 0) > $idleswaptimeout) {
	$errors["Idleswap"] = "Invalid time provided - ".
	    "must be non-zero and less than $idleswaptimeout";
    }
    $command_options .= " -l " . (60 * $formfields[exp_idleswap_timeout]);
}

#
# Autoswap.
#
if (!isset($formfields[exp_autoswap]) ||
    strcmp($formfields[exp_autoswap], "1")) {
    $formfields[exp_autoswap] = 0;
}
else {
    $formfields[exp_autoswap] = 1;
    
    if (!isset($formfields[exp_autoswap_timeout]) ||
	!preg_match("/^[\d]+$/", $formfields[exp_idleswap_timeout]) ||
	($formfields[exp_autoswap_timeout] + 0) <= 0) {
	$errors["Max. Duration"] = "No or invalid time provided";
    }
    else {
	$command_options .= " -a " . (60 * $formfields[exp_autoswap_timeout]);
    }
}

#
# Save Disk
#
if (isset($formfields[exp_savedisk]) &&
    strcmp($formfields[exp_savedisk], "Yep") == 0) {
    $command_options .= " -s";
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

#
# Batchmode
#
if (isset($formfields[batched]) && $formfields[batched] == "Yep") {
    $command_options .= " -b";
    $batchmode = 1;
}

if (count($errors)) {
    SPITFORM($formfields, $parameters, $errors);
    PAGEFOOTER();
    exit(1);
}

#
# Grab the unix GID for running scripts.
#
TBGroupUnixInfo($pid, $gid, $unix_gid, $unix_name);

#
# Avoid SIGPROF in child.
#
set_time_limit(0);

# Okay, we can spit back a header now that there is no worry of redirect.
PAGEHEADER("Instantiate Experiment Template");

echo "<font size=+2>Experiment Template <b>" .
        MakeLink("template",
		 "guid=$guid&version=$version", "$guid/$version") . 
      "</b></font>\n";
echo "<br><br>\n";

echo "<b>Starting template instantiation!</b> ... ";
echo "this will take several minutes at least; please be patient.";
echo "<br><br>\n";
flush();

#
# Run the backend script.
#
$retval = SUEXEC($uid, "$pid,$unix_gid",
		 "webtemplate_swapin $command_options -e $eid $guid/$version",
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
# This does both the log output, and the state change watcher popup
#
if ($batchmode) {
    echo "You template instantation has been queued and will run when
          enough resources become available. This might happen
          immediately, or it may take hours or days; you will be
          notified via email when insantiation is complete, and again when
          your experiment has completed.  In the meantime, you can check the
          progress on the <A href='showexp.php3?pid=$pid&eid=$eid'>web page</A>
          to see how many attempts have been made, and when the
          last attempt was.\n";
  
    STARTWATCHER($pid, $eid);
}
else {
    STARTLOG($pid, $eid);
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
