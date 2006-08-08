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
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

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

#
# Spit out the image with a content header.
#
$eid    = $instance->eid();
$pid    = $instance->pid();
$runidx = $instance->runidx();
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

if (!TBExptGroup($pid, $eid, $gid)) {
    TBERROR("No such experiment $pid/$eid!", 1);
}     
TBGroupUnixInfo($pid, $gid, $unix_gid, $unix_name);

if ($fp = popen("$TBSUEXEC_PATH $uid $unix_name webtemplate_linkgraph " .
		"-i $exptidx -r $runidx $guid/$vers $which", "r")) {
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
