<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
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
				 "confirmed", PAGEARG_BOOLEAN,
				 "spew",      PAGEARG_BOOLEAN);
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
$unix_gid = $template->UnixGID();
$exptidx  = $instance->exptidx();

if (! $template->AccessCheck($this_user, $TB_EXPT_MODIFY)) {
    USERERROR("You do not have permission to export in template ".
	      "$guid/$vers!", 1);
}

if (!isset($confirmed)) {
    PAGEHEADER("Template Export");
    
    echo "<center><br><font size=+1>
          Export instance $exptidx in Template
             in Template $guid/$vers?</font>\n";
    
    $template->Show();

    $url = CreateURL("template_export", $instance);
    
    echo "<form action='$url' method=post>\n";
    echo "<br>\n";
    echo "<input type=checkbox name=spew value=1>
                Save to local disk? [<b>1</b>]\n";
    echo "<br>\n";
    echo "<br>\n";
    echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
    echo "</form>\n";
    echo "</center>\n";
    echo "<blockquote><blockquote>
          <ol>
            <li> By default, your instance will be exported to
                 <tt>$TBPROJ_DIR/$pid/export/$guid/$vers/$exptidx</tt>,
                 available to
                 other experiments in your project. If you want to export
                 to a local file, click the <em>local disk</em> option
                 and a .tgz file will be generated and sent to your browser.
          </ol>
          </blockquote></blockquote>\n";
    
    PAGEFOOTER();
    return;
}

# Check spew option.
if (isset($spew) && $spew) {
    $spew = 1;
}
else {
    $spew = 0;
}

#
# Avoid SIGPROF in child.
#
set_time_limit(0);

if (! $spew) {
    PAGEHEADER("Template Export");

    echo $template->PageHeader();
    echo "<br><br>\n";

    echo "<script type='text/javascript' language='javascript' ".
	"        src='template_sup.js'>\n";
    echo "</script>\n";

    STARTBUSY("Starting export");
    sleep(1);
}

#
# A cleanup function to keep the child from becoming a zombie, since
# the script is terminated, but the children are left to roam.
#
$fp = 0;

function SPEWCLEANUP()
{
    global $fp;

    if (!$fp || !connection_aborted()) {
	exit();
    }
    pclose($fp);
    exit();
}

#
# Run the backend script.
#
if ($spew) {
    ignore_user_abort(1);
    register_shutdown_function("SPEWCLEANUP");

    if ($fp = popen("$TBSUEXEC_PATH $uid $pid,$unix_gid ".
		    "  webtemplate_export -s -i $exptidx $guid/$vers",
		    "r")) {
	header("Content-Type: application/x-tar");
	header("Content-Encoding: x-gzip");
	header("Content-Disposition: attachment; filename=$exptidx.tgz");
	header("Expires: Mon, 26 Jul 1997 05:00:00 GMT");
	header("Cache-Control: no-cache, must-revalidate");
	header("Pragma: no-cache");
	flush();

	while (!feof($fp)) {
	    $string = fgets($fp, 1024);
	    echo "$string";
	    flush();
	}
	pclose($fp);
	$fp = 0;
    }
    else {
	USERERROR("Could not export", 1);
    }
    return;
}

#
# Standard mode ...
#
$retval = SUEXEC($uid, "$pid,$unix_gid",
		 "webtemplate_export " . ($spew ? "-s" : "") .
		 "-i $exptidx $guid/$vers",
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

# Zap back to template page.
PAGEREPLACE(CreateURL("template_show", $template));

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
