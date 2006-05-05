<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("template_defs.php");

#
# Only known and logged in users.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);


#
# Spit the form out using the array of data.
#
function SPITFORM($formfields, $errors)
{
    global $TBDB_PIDLEN, $TBDB_GIDLEN, $TBDB_EIDLEN, $TBDOCBASE;
    global $guid, $version;

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

    echo "<font size=+2>Experiment Template <b>" .
            MakeLink("template",
		     "guid=$guid&version=$version", "$guid/$version") . 
	   "</b></font>\n";
    echo "<br><br>\n";

    echo "<form action='template_modify.php?guid=$guid&version=$version'
                method='post'>";
    echo "<table align=center border=1>\n";

    #
    # TID:
    #
    echo "<tr>
              <td class='pad4'>Template ID:
              <br><font size='-1'>(alphanumeric, no blanks)</font></td>
              <td class='pad4' class=left>
                  <input type=text
                         name=\"formfields[tid]\"
                         value=\"" . $formfields[tid] . "\"
	                 size=$TBDB_EIDLEN
                         maxlength=$TBDB_EIDLEN>
              &nbsp (optional; we will generate one for you).
              </td>
          </tr>\n";

    echo "<tr>
              <td colspan=2>
               Use this text area to optionally describe your template:
              </td>
          </tr>
          <tr>
              <td colspan=2 align=center class=left>
                  <textarea name=\"formfields[description]\"
                    rows=4 cols=100>" .
	            ereg_replace("\r", "", $formfields[description]) .
	           "</textarea>
              </td>
          </tr>\n";


    echo "<tr>
              <td colspan=2>
              <textarea name=\"formfields[nsdata]\"
                   cols=100 rows=40>" . $formfields[nsdata] . "</textarea>
              </td>
          </tr>\n";

    echo "<tr>
              <td class='pad4' align=center colspan=2>
                <b><input type=submit name=modify value='Modify Template'></b>
              </td>
         </tr>
        </form>
        </table>\n";
}

#
# Standard Testbed Header
#
PAGEHEADER("Modify Experiment Template");

#
# Verify page arguments.
# 
if (!isset($guid) ||
    strcmp($guid, "") == 0) {
    USERERROR("You must provide a Template ID.", 1);
}
if (!isset($version) ||
    strcmp($version, "") == 0) {
    USERERROR("You must provide a Template version", 1);
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
if (! TBExptTemplateAccessCheck($uid, $guid, $TB_EXPT_MODIFY)) {
    USERERROR("You do not have permission to modify experiment template ".
	      "$guid/$version!", 1);
}

#
# Put up the modify form on first load.
# 
if (! isset($modify)) {
    #
    # Grab NS file for the template.
    #
    $input_list  = TBTemplateInputFiles($guid, $version);
    TBTemplateDescription($guid, $version, $description);

    $defaults = array();
    $defaults["nsdata"] = $input_list[0];
    $defaults["description"] = $description;
    SPITFORM($defaults, 0);
    PAGEFOOTER();
    exit();
}
elseif (! isset($formfields)) {
    PAGEARGERROR();
}

#
# Okay, validate form arguments.
#
$errors       = array();
$command_args = "";

#
# Generate a hopefully unique filename that is hard to guess. See below.
# 
list($usec, $sec) = explode(' ', microtime());
srand((float) $sec + ((float) $usec * 100000));
$foo = rand();

#
# Get template group so we can get the unix_gid.
#
if (! TBGuid2PidGid($guid, $pid, $gid)) {
    TBERROR("Could not get pid,gid for experiment template $guid/$version", 1);
}
    
#
# TID
#
if (!isset($formfields[tid]) || $formfields[tid] == "") {
    #
    # Generate a unique one.
    #
    $tid = "T" . substr(md5(uniqid($foo, true)), 0, 10);
}
elseif (!TBvalid_eid($formfields[tid])) {
    $errors["Template ID"] = TBFieldErrorString();
}
elseif (TBValidExperimentTemplate($pid, $formfields[tid])) {
    $errors["Template ID"] = "Already in use";
}
else {
    $tid = $formfields[tid];
}

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
    $command_args .= " -E " . escapeshellarg($formfields[description]);
}

#
# NS File.
#
if (!isset($formfields[nsdata]) || $formfields[nsdata] == "") {
    $errors["NS File"] = "Missing Field";
}

if (count($errors)) {
    SPITFORM($formfields, $errors);
    PAGEFOOTER();
    exit(1);
}

echo "<script type='text/javascript' language='javascript' ".
     "        src='template_sup.js'>\n";
echo "</script>\n";

#
# Generate a unique and hard to guess filename, and write NS to it.
#
$nsfile = "/tmp/$uid-$foo.nsfile";

if (! ($fp = fopen($nsfile, "w"))) {
    TBERROR("Could not create temporary file $nsfile", 1);
}
fwrite($fp, $formfields[nsdata]);
fclose($fp);
chmod($nsfile, 0666);

# Need this for running scripts.
TBGroupUnixInfo($pid, $gid, $unix_gid, $unix_name);

echo "<center>\n";
echo "<b>Starting template modification!</b> ...<br>\n";
echo "This will take a few moments; please be patient.<br>\n";
echo "<br><br>\n";
echo "<img id='busy' src='busy.gif'><span id='loading'> Working ...</span>";
echo "<br><br>\n";
echo "</center>\n";
flush();

# And run that script!
$retval = SUEXEC($uid, "$pid,$unix_gid",
		 "webtemplate_create -w -q ".
		 "-m $guid/$version $command_args $pid $tid $nsfile",
		 SUEXEC_ACTION_IGNORE);

unlink($nsfile);

/* Clear the various 'loading' indicators. */
echo "<script type='text/javascript' language='javascript'>\n";
echo "ClearLoadingIndicators();\n";
echo "</script>\n";

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

unset($guid);
if (TBPidTid2Template($pid, $tid, $guid, $version)) {
    echo "<script type='text/javascript' language='javascript'>\n";
    echo "PageReplace('template_show.php?guid=$guid&version=$version');\n";
    echo "</script>\n";
}

#
# In case the above redirect fails.
#
echo "Done!";
echo "<br><br>\n";

if (isset($guid)) {
    SHOWTEMPLATE($guid, $version);
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
