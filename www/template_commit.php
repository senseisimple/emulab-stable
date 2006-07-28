<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("template_defs.php");

#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

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

# Canceled operation redirects back to template page.
if ($canceled) {
    header("Location: template_show.php?guid=$guid&version=$version");
    return;
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
    USERERROR("You do not have permission to commit experiment template ".
	      "$guid/$version!", 1);
}

#
# Check to make sure a valid instance that is actually swapped in.
#
$instance = TemplateInstance::LookupByExptidx($exptidx);
if (!$instance) {
    TBERROR("Template Instance $guid/$version/$exptidx is not ".
	    "a valid experiment template instance!", 1);
}

# Need these below.
$pid = $template->pid();
$gid = $template->gid();
$eid = $instance->eid();

#
# Confirm
#
if (!$confirmed) {
    PAGEHEADER("Create Template from Instance");
    
    echo "<center><br><font size=+1>
          Create new Template from instance $eid 
             in Template $guid/$version?</font>\n";
    
    $template->Show();
    echo "<br>";
    $instance->Show(0);

    echo "<form action='template_commit.php?guid=$guid&version=$version".
	"&exptidx=$exptidx' method=post>\n";

    echo "<br>\n";
    echo "<br>\n";
    echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
    echo "</form>\n";
    echo "</center>\n";
    
    PAGEFOOTER();
    return;
}

#
# We need the unix gid for the project for running the scripts below.
# Note usage of default group in project.
#
TBGroupUnixInfo($pid, $gid, $unix_gid, $unix_name);

#
# Avoid SIGPROF in child.
#
set_time_limit(0);

PAGEHEADER("Create Template from Instance");

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

STARTBUSY("Starting export");
sleep(1);

#
# Run script.
#
$retval = SUEXEC($uid, "$pid,$unix_gid",
		 "webtemplate_commit -e $eid $guid/$version",
		 SUEXEC_ACTION_IGNORE);

/* Clear the 'loading' indicators above */
if ($retval) {
    CLEARBUSY();
}
else {
    STOPBUSY();
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

# Zap to the newly created template page.
$template->Refresh();

PAGEREPLACE("template_show.php?guid=$guid&version=". $template->child_vers());

#
# In case the above fails.
#
echo "<center><b>Done!</b></center>";
echo "<br><br>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
