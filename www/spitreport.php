<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$reqargs = RequiredPageArguments("experiment", PAGEARG_EXPERIMENT);
$optargs = OptionalPageArguments("showevents", PAGEARG_BOOLEAN);

# Need these below.
$pid = $experiment->pid();
$eid = $experiment->eid();

#
# Verify permission.
#
if (!$experiment->AccessCheck($this_user, $TB_EXPT_READINFO)) {
    USERERROR("Not enough permission to view experiment $pid/$eid", 1);
}

$output = array();
$retval = 0;

if (isset($showevents) && $showevents) {
    $flags = "-v";
}
else {
    # Show event summary and firewall info.
    $flags = "-b -e -f";
}

$result =
    exec("$TBSUEXEC_PATH $uid $TBADMINGROUP webtbreport $flags $pid $eid",
	 $output, $retval);

header("Content-Type: text/plain");
for ($i = 0; $i < count($output); $i++) {
    echo "$output[$i]\n";
}
echo "\n";
