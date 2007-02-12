<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Request a Swap/Terminate");

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Only admins can do this!
#
if (!$isadmin) {
    USERERROR("Only TB admins can do this!", 1);
}

#
# Verify page arguments.
#
$reqargs = RequiredPageArguments("experiment", PAGEARG_EXPERIMENT);
$optargs = OptionalPageArguments("canceled",   PAGEARG_STRING,
				 "confirmed",  PAGEARG_STRING,
				 "force",      PAGEARG_BOOLEAN);

#
# Need these below
#
$pid = $experiment->pid();
$eid = $experiment->eid();
$pcs = $experiment->PCCount();

echo $experiment->PageHeader();
echo "<br>";

#
# We run this twice. The first time we are checking for a confirmation
# by putting up a form. The next time through the confirmation will be
# set. Or, the user can hit the cancel button, in which case we should
# probably redirect the browser back up a level.
#
if (isset($canceled) && $canceled) {
    echo "<center><h3>Swap request canceled!</h3>
          </center>\n";
    PAGEFOOTER();
    return;
}

# We only send to the proj leader after we have sent $tell_proj_head requests
$tell_proj_head = TBGetSiteVar("idle/cc_grp_ldrs");
$q = DBQueryWarn("select swappable, swap_requests,
                  date_format(last_swap_req,\"%T\") as lastreq
                  from experiments
                  where eid='$eid' and pid='$pid'");
$r = mysql_fetch_array($q);

$swappable     = $experiment->swappable();
$swap_requests = $experiment->swap_requests();
$last_swap_req = $experiment->last_swap_req();

if (!isset($confirmed)) {
    echo "<center><h3>
           Expt. '$eid' in project '$pid' has $pcs Emulab node(s) ".
     "reserved,<br>\nand has been sent $swap_requests swap request".
      ($swap_requests!=1?"s":"")." since it went idle.<br>\n";
    if ($swap_requests > 0) {
      echo "The most recent one was sent at $last_swap_req.<br>\n";
    }
    if ($swap_requests >= $tell_proj_head) {
      echo "This notification will also be sent to the project leader.\n";
    }
    echo "<p>
          Are you sure you want to send an email message requesting that
          experiment '$eid' be swapped or terminated?</p>
          </h3>\n";

    #
    # Dump experiment record.
    #
    $experiment->Show();

    $url = CreateURL("request_swapexp", $experiment);

    echo "<form action='$url' method=post>";
    echo "<p><input type=checkbox name=force value=1>Force: ".
	"Send message even if not idle</p>\n";
    echo "<input type=submit name=confirmed value=Confirm>\n";
    echo "<input type=submit name=canceled value=Cancel>\n";
    echo "</form>\n";
    echo "</center>\n";

    PAGEFOOTER();
    return;
}

# Confirmed and ready to go...
if (isset($force) && $force) {
    $force="-f";
}
else {
    $force="";
}

STARTBUSY("Sending request");
SUEXEC($uid, $TBADMINGROUP,
       "webidlemail $force $pid $eid", SUEXEC_ACTION_IGNORE);
CLEARBUSY();

if ($suexec_retval == 0) {
    echo "<p><center>
         Message successfully sent!
         </center></p>\n";
} elseif ($suexec_retval == 2) {
    $url = CreateURL("request_swapexp", $experiment);

    echo "<p><center>
         No message was sent, because it wasn't time for a message.<br>
	 Use the <a href=\"$url\">Force</a> option to send a message anyway.
         </center></p>\n";
} else {
    echo "<p><center>
	 There was an unknown problem sending the message.<br>
	 Output was:<pre>$suexec_output</pre>
         </center></p>\n";
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
