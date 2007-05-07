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
$optargs = OptionalPageArguments("instance",  PAGEARG_INSTANCE,
				 "template",  PAGEARG_TEMPLATE,
				 "canceled",  PAGEARG_BOOLEAN,
				 "confirmed", PAGEARG_BOOLEAN,
				 "runidx",    PAGEARG_INTEGER,
				 "tag",       PAGEARG_STRING,
				 "overwrite", PAGEARG_BOOLEAN,
				 "spew",      PAGEARG_BOOLEAN);

#
# An instance might be a current or historical. If its a template, use
# the underlying experiment of the template.
#
if (isset($instance)) {
    $template = $instance->GetTemplate();
}
elseif (! isset($template)) {
    PAGEARGERROR("Must provide a template or an instance to export");
}

if (isset($canceled) && $canceled) {
    if (isset($instance))
	header("Location: ". CreateURL("template_show", $template));
    else
	header("Location: ". CreateURL("template_show", $instance));	
    return;
}

# Need these below.
$guid     = $template->guid();
$vers     = $template->vers();
$pid      = $template->pid();
$unix_gid = $template->UnixGID();

if (! $template->AccessCheck($this_user, $TB_EXPT_READINFO)) {
    USERERROR("You do not have permission to export from template ".
	      "$guid/$vers!", 1);
}
if (isset($instance)) {
    $exptidx  = $instance->exptidx();
    
    if (isset($runidx) && !$instance->ValidRun($runidx)) {
	USERERROR("Run '$runidx' is not a valid run in instance '$exptidx'",
		  1);
    }
}

function SPITFORM($error)
{
    global $template, $instance, $runidx, $TBPROJ_DIR;
    global $tag, $pid, $guid, $vers;

    if ($error) {
	echo "<center>\n";
	echo "<font size=+1 color=red>";
	echo $error;
	echo "</font><br>\n";
    }
    else {
	echo $template->PageHeader();
	echo "<br><br>\n";
    
	echo "<center>\n";
	echo "<font size=+1>";

	if (isset($instance)) {
	    echo "Export instance " . $instance->exptidx() .
	           (isset($runidx) ? " (Run $runidx)" : "") . "?";
	}
	else {
	    echo "Export Template Datastore";
	}
	echo "</font>";
    }
    
    $template->Show();

    if (isset($instance)) {
	$url = CreateURL("template_export", $instance);
	if (isset($runidx)) {
	    $url .= "&runidx=$runidx";
	}
    }
    else {
	$url = CreateURL("template_export", $template);
    }
    if (isset($tag)) {
	$url .= "&tag=$tag";
    }
    
    echo "<form action='$url' method=post>\n";
    echo "<br>\n";
    echo "<input type=checkbox name=spew value=1>
                Save to local disk? [<b>1</b>]\n";
    echo "<br>\n";
    echo "<input type=checkbox name=overwrite value=1>
                Overwrite existing export on server? [<b>2</b>]\n";
    echo "<br>\n";
    echo "<br>\n";
    echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
    echo "</form>\n";
    echo "</center>\n";
    echo "<blockquote><blockquote>
          <ol>
            <li> By default, your export will be saved to
                 <tt>$TBPROJ_DIR/$pid/export/$guid/$vers</tt>,
                 available to
                 other experiments in your project. If you want to export
                 to a local file, click the <em>local disk</em> option
                 and a .tgz file will be generated and sent to your browser.

            <li> When exporting to the above mentioned directory, overwrite
                 any existing export. Otherwise an error will be reported.
          </ol>
          </blockquote></blockquote>\n";
    return;
}
if (!isset($confirmed)) {
    PAGEHEADER("Template Export");
    SPITFORM(null);
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
if (isset($overwrite) && $overwrite) {
    $overwrite = 1;
}
else {
    $overwrite = 0;
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
$export_args = (isset($tag) ? "-t " . escapeshellarg($tag) . " " : "");

if (isset($instance)) {
    $export_args .= "-i $exptidx";
    if (isset($runidx))
	$exptidx .=  " -r " . escapeshellarg($runidx);
}
else {
    $export_args .= "$guid/$vers";
}
if ($overwrite) {
     $export_args = "-o " . $export_args;
}

if ($spew) {
    ignore_user_abort(1);
    register_shutdown_function("SPEWCLEANUP");

    if (($fp = popen("$TBSUEXEC_PATH $uid $pid,$unix_gid ".
		     "  webtemplate_export -s $export_args",
		     "r"))) {
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
		 "webtemplate_export $export_args",
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
    if ($retval == 2) {
	SPITFORM("Export directory already exists! Use the overwrite option.");
	PAGEFOOTER();
	return;
    }
    SUEXECERROR(SUEXEC_ACTION_USERERROR);
    return;
}

# Zap back to template page.
PAGEREPLACE(CreateURL("template_show", $template));

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
