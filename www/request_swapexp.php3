<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003, 2005, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

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
# Verify page arguments.
# 
if (!isset($pid) ||
    strcmp($pid, "") == 0) {
    USERERROR("You must provide a Project ID.", 1);
}

if (!isset($eid) ||
    strcmp($eid, "") == 0) {
    USERERROR("You must provide an Experiment ID.", 1);
}

#
# Only admins can do this!
#
if (! !$isadmin) {
    USERERROR("Only TB admins can do this!", 1);
}

#
# Check to make sure this is a valid PID/EID tuple.
#
if (! TBValidExperiment($pid, $eid)) {
    USERERROR("The experiment $eid is not a valid experiment ".
	      "in project $pid.", 1);
}

#
# We run this twice. The first time we are checking for a confirmation
# by putting up a form. The next time through the confirmation will be
# set. Or, the user can hit the cancel button, in which case we should
# probably redirect the browser back up a level.
#
if ($canceled) {
    echo <<<END
<center><h2>Swap request canceled!</h2>
<p>
<a href="showexp.php3?pid=$pid&eid=$eid">Back to experiment $pid/$eid</a>
</p></center>
END;
    
    PAGEFOOTER();
    return;
}

# We only send to the proj leader after we've sent $tell_proj_head requests
$tell_proj_head = TBGetSiteVar("idle/cc_grp_ldrs");
$q = DBQueryWarn("select swappable, swap_requests,
                  date_format(last_swap_req,\"%T\") as lastreq
                  from experiments
                  where eid='$eid' and pid='$pid'");
$r = mysql_fetch_array($q);
$swappable = $r["swappable"];
$swap_requests = $r["swap_requests"];
$last_swap_req = $r["lastreq"];

$q=DBQueryWarn("select count(*) as c from reserved as r
left join nodes as n on r.node_id=n.node_id 
left join node_types as nt on n.type=nt.type 
where nt.class='pc' and pid='$pid' and eid='$eid'");

$r=mysql_fetch_array($q);
$c=$r["c"];

if (!$confirmed) {
    echo "<center><h3>
Expt. '$eid' in project '$pid' has $c Emulab node".($c!=1?"s":"").
      " reserved,<br>\nand has been sent $swap_requests swap request".
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
    SHOWEXP($pid, $eid);
    
    echo "<form action='request_swapexp.php3?pid=$pid&eid=$eid' method=post>";
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
if ($force) {
    $force="-f";
} else {
    $force="";
}

SUEXEC($uid, $TBADMINGROUP,
       "webidlemail $force $pid $eid", SUEXEC_ACTION_IGNORE);
# sets some globals for us...

if ($suexec_retval == 0) {
    echo "<p><center>
         Message successfully sent!
         </center></p>\n";
} elseif ($suexec_retval == 2) {
    echo "<p><center>
         No message was sent, because it wasn't time for a message.<br>
	 Use the <a href=\"request_swapexp.php3?pid=$pid&eid=$eid\">Force</a>
	 option to send a message anyway.
         </center></p>\n";
} else {
    echo "<p><center>
	 There was an unknown problem sending the message.<br>
	 Output was:<pre>$suexec_output</pre>
         </center></p>\n";
}

echo <<<END
<p><center>
<a href="showexp.php3?pid=$pid&eid=$eid">Back to experiment $pid/$eid</a>
</center></p>
END;

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
