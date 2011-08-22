<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2011 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("template_defs.php");

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$reqargs = RequiredPageArguments("instance",  PAGEARG_INSTANCE);
$optargs = OptionalPageArguments("canceled",  PAGEARG_BOOLEAN,
				 "referrer",  PAGEARG_STRING,
				 "confirmed", PAGEARG_BOOLEAN);
$template = $instance->GetTemplate();

if (isset($canceled) && $canceled) {
    header("Location: ". CreateURL("template_show", $template));
    return;
}

# Need these below.
$guid = $template->guid();
$vers = $template->vers();
$pid  = $template->pid();
$eid  = $instance->eid();
$iid  = $instance->id();
$unix_gid = $template->UnixGID();
$exptidx  = $instance->exptidx();
$project  = $template->GetProject();
$unix_pid = $project->unix_gid();

if (! $template->AccessCheck($this_user, $TB_EXPT_READINFO)) {
    USERERROR("You do not have permission to view experiment template ".
	      "$guid/$vers!", 1);
}

if (!isset($confirmed)) {
    PAGEHEADER("Reconstitute");

    echo $instance->PageHeader();
    
    echo "<br><br><center><br><font size=+1>
          Reconstitute database(s) for instance $iid?</font>\n";
    
    $template->Show();

    $url = CreateURL("template_analyze", $instance);

    if (!isset($referrer))
	$referrer = $_SERVER['HTTP_REFERER'];

    echo "<form action='$url' method=post>\n";
    echo "<br>\n";
    echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
    echo "<input type=hidden name=referrer value=$referrer>\n";
    echo "</form>\n";
    echo "</center>\n";
    
    PAGEFOOTER();
    return;
}

#
# Avoid SIGPROF in child.
#
set_time_limit(0);

PAGEHEADER("Reconstitute");

echo $template->PageHeader();
echo "<br><br>\n";

echo "<script type='text/javascript' src='template_sup.js'>\n";
echo "</script>\n";

STARTBUSY("Starting Database Reconstitution");
sleep(1);

#
# Run the backend script
#
$retval = SUEXEC($uid, "$unix_pid,$unix_gid",
		 "webtemplate_analyze -i $exptidx $guid/$vers",
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

if (!isset($referrer)) {
    $referrer = CreateURL("template_show", $template);
}
# Zap back to template page.
PAGEREPLACE($referrer);

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
