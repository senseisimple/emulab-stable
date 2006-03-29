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

echo "<font size=+2>Experiment Template <b>$guid/$version</b>".
     "</font>\n";
echo "<br><br>\n";
flush();


#
# Put up the modify form on first load.
# 
if (! isset($modify)) {
    #
    # Grab NS file for the template.
    #
    $input_list = TBTemplateInputFiles($guid, $version);
    
    echo "<form action='template_modify.php?guid=$guid&version=$version'
                method='post'>";
    echo "<textarea cols='100' rows='40' name='nsdata'>";

    #
    # Display only first one for now.
    #
    while (list($input_idx, $input_file) = each($input_list)) {
	echo "$input_file";
    }
    echo "</textarea>";
    echo "<br>";
    echo "<input type='submit' name='modify' value='Modify Template'>";
    echo "</form>\n";
    PAGEFOOTER();
    exit();
}

#
# Okay, form has been submitted.
#
if (! isset($nsdata)) {
    USERERROR("NSdata CGI variable missing (How did that happen?)", 1);
}

#
# Generate a hopefully unique filename that is hard to guess.
# See backend scripts.
# 
list($usec, $sec) = explode(' ', microtime());
srand((float) $sec + ((float) $usec * 100000));
$foo = rand();
    
$nsfile = "/tmp/$uid-$foo.nsfile";

if (! ($fp = fopen($nsfile, "w"))) {
    TBERROR("Could not create temporary file $nsfile", 1);
}
$nsdata_string = $nsdata;
fwrite($fp, $nsdata_string);
fclose($fp);
chmod($nsfile, 0666);

#
# Get template group so we can get the unix_gid.
#
if (! TBGuid2PidGid($guid, $pid, $gid)) {
    TBERROR("Could not get pid,gid for experiment template $guid/$version", 1);
}
TBGroupUnixInfo($pid, $gid, $unix_gid, $unix_name);

#
# XXX Generate a new and hopefully unique tid so that we can find this thing
# later. This needs more thought.
#
$tid = "T" . substr(md5(uniqid($foo, true)), 0, 10);

echo "<b>Starting template modification!</b> ... ";
echo "this will take a few moments; please be patient.";
echo "<br><br>\n";
flush();

# And run that script!
$retval = SUEXEC($uid, "$pid,$unix_gid",
		 "webtemplate_create -w -q ".
		 "-m $guid/$version $pid $tid $nsfile",
		 SUEXEC_ACTION_IGNORE);

unlink($nsfile);

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

echo "Done!";
echo "<br><br>\n";

if (TBPidTid2Template($pid, $tid, $guid, $version)) {
    SHOWTEMPLATE($guid, $version);
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
