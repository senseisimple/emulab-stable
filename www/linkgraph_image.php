<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("template_defs.php");

#
# Only known and logged in users ...
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
# 
if (!isset($exptidx) ||
    strcmp($exptidx, "") == 0) {
    USERERROR("You must provide an instance ID", 1);
}
if (!TBvalid_integer($exptidx)) {
    PAGEARGERROR("Invalid characters in instance ID!");
}

# Only template instances right now.
$instance = TemplateInstance::LookupByExptidx($exptidx);
if (!$instance) {
    USERERROR("The instance $exptidx is not a valid instance!", 1);
}
$template = $instance->template();

# Optional runidx for graphing just a specific run.
$runarg = "";
$vnodes = "";

if (isset($runidx) && $runidx != "") {
    if (!TBvalid_integer($runidx)) {
	PAGEARGERROR("Invalid characters in run index!");
    }
    else {
	$runarg = "-r $runidx";
    }
}

# Optional src and dst arguments.
if (isset($srcvnode) && $srcvnode != "") {
    if (! preg_match("/^[-\w\/]*$/", $srcvnode)) {
	PAGEARGERROR("Invalid characters in src vnode!");
    }
    else {
	$vnodes .= " -s " . escapeshellarg($srcvnode);
    }
}
if (isset($dstvnode) && $dstvnode != "") {
    if (! preg_match("/^[-\w\/]*$/", $dstvnode)) {
	PAGEARGERROR("Invalid characters in dst vnode!");
    }
    else {
	$vnodes .= " -t " . escapeshellarg($dstvnode);
    }
}

#
# Spit out the image with a content header.
#
$eid    = $instance->eid();
$pid    = $instance->pid();
$gid    = $template->gid();
$guid   = $instance->guid();
$vers   = $instance->vers();
$which  = "pps";

#
# Default to pps if no graphtype.
#
if (isset($graphtype) && $graphtype != "") {
    if (! preg_match('/^[\w]*$/', $graphtype)) {
	PAGEARGERROR("Invalid characters in graphtype!");
    }
    $which = $graphtype;
}

TBGroupUnixInfo($pid, $gid, $unix_gid, $unix_name);

if ($fp = popen("$TBSUEXEC_PATH $uid $unix_name webtemplate_linkgraph " .
		"-i $exptidx $runarg $vnodes $guid/$vers $which", "r")) {
    header("Content-type: image/gif");
    fpassthru($fp);
}
else {
    # No Data. Spit back a stub image.
    header("Content-type: image/gif");
    readfile("coming-soon-thumb.gif");
}

#
# No Footer!
# 
?>
