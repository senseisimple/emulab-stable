<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Request info about possibly Idle experiment");

#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

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
if (! ISADMIN($uid)) {
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
    echo "<center><h2>Idle info request canceled!</h2>
          <br><br>
          <a href='showexp.php3?pid=$pid&eid=$eid'>
             Back to experiment $pid/$eid</a>
          </center>\n";
    PAGEFOOTER();
    return;
}

#
# See how many PCs the experiment is holding.
# 
$query_result =
    DBQueryFatal("select count(*) as count from reserved as r ".
		 "left join nodes as n on r.node_id=n.node_id ".
		 "left join node_types as nt on n.type=nt.type ".
		 "where nt.class='pc' and pid='$pid' and eid='$eid'");

if (!mysql_num_rows($query_result)) {
    echo "<center><h2>Idle info request canceled cause there are no PCs!</h2>
          <br><br>
          <a href='showexp.php3?pid=$pid&eid=$eid'>
             Back to experiment $pid/$eid</a>
          </center>\n";
    PAGEFOOTER();
    return;
}
$row = mysql_fetch_array($query_result);
$pcs = $row["count"];

if (!$confirmed) {
    echo "<br><center><h3>
          Are your <b>SURE</b> you want to send an Idle Info request
          email message to the swapper of $pid/$eid?
          </h3></center>\n";

    #
    # Dump experiment record.
    # 
    SHOWEXP($pid, $eid);
    
    echo "<form action='request_idleinfo.php3?pid=$pid&eid=$eid' method=post>";
    echo "<input type=submit name=confirmed value=Confirm>\n";
    echo "<input type=submit name=canceled value=Cancel>\n";
    echo "</form>\n";
    echo "</center>\n";

    PAGEFOOTER();
    return;
}

# Info about experiment.
$query_result =
    DBQueryFatal("select e.expt_swap_uid as swapper, ".
		 "       UNIX_TIMESTAMP(now())-UNIX_TIMESTAMP(e.expt_swapped)".
		 "   as swapseconds, r.pnodes ".
		 " from experiments as e ".
		 "left join experiment_stats as s on s.exptidx=e.idx ".
		 "left join experiment_resources as r on ".
		 "     s.rsrcidx=r.idx ".
		 "where e.pid='$pid' and e.eid='$eid'");

$row = mysql_fetch_array($query_result);
$swapper = $row["swapper"];
$pcs     = $row["pnodes"];
$seconds = $row["swapseconds"];
$hours   = intval($seconds / 3600);

# Swapper email.
$user_name  = "";
$user_email = "";
TBUserInfo($swapper, $user_name, $user_email);

# And send email
TBMAIL("$user_name <$user_email>",
       "Please tell us about your experiment",
       "Hi. We noticed that your experiment '$pid/$eid' has been swapped\n".
       "in for $hours hours. Such long running experiments are unusual\n".
       "so we want to make sure this experiment is still doing useful work.\n".
       "\n".
       "Please respond to this message letting us know; if we do not hear\n".
       "from you within a couple of hours, we will be forced to swap your\n".
       "experiment out so that others can use the nodes.\n\n".
       "Thanks very much!\n",
       "From: $TBMAIL_OPS\n".
       "Bcc: $TBMAIL_OPS\n".
       "Errors-To: $TBMAIL_WWW");

echo "<center><h2>Message sent!</h2>
      <br><br>
      <a href='showexp.php3?pid=$pid&eid=$eid'>Back to experiment $pid/$eid</a>
      </center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
