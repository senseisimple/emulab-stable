<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Request a Swap/Terminate");

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
# Check to make sure this is a valid PID/EID tuple.
#
if (! TBValidExperiment($pid, $eid)) {
  USERERROR("The experiment $eid is not a valid experiment ".
            "in project $pid.", 1);
}

#
# Only admins can do this!
#
if (! ISADMIN($uid)) {
    USERERROR("Only TB admins can do this!", 1);
}

#
# We run this twice. The first time we are checking for a confirmation
# by putting up a form. The next time through the confirmation will be
# set. Or, the user can hit the cancel button, in which case we should
# probably redirect the browser back up a level.
#
if ($canceled) {
    echo "<center><h2><br>
          Swap/Termination request canceled!
          </h2></center>\n";
    
    PAGEFOOTER();
    return;
}

# We only send to the proj leader after we've sent $tell_proj_head requests
$tell_proj_head = 2;
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
    echo "<center><h3><br>
Expt. '$eid' in project '$pid' has $c Emulab node".($c!=1?"s":"").
      " reserved,<br>and has been sent $swap_requests swap request".
      ($swap_requests!=1?"s":"")." since it went idle.<br>";
    if ($swap_requests > 0) {
      echo "The most recent one was sent at $last_swap_req.<br>";
    }
    if ($swap_requests >= $tell_proj_head) {
      echo "This notification will also be sent to the project leader.<br>";
    }
    echo "<p>
          Are you sure you want to send an email message requesting that<br>
          experiment '$eid' be swapped or terminated?
          </h3>\n";

    #
    # Dump experiment record.
    # 
    SHOWEXP($pid, $eid);
    
    echo "<form action='request_swapexp.php3?pid=$pid&eid=$eid' method=post>";
    echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
    echo "</form>\n";
    echo "</center>\n";

    PAGEFOOTER();
    return;
}

#
# Grab experiment/project leaders and email info.
#
$expleader  = 0;
$projleader = 0;

if (! TBExpLeader($pid, $eid, $expleader)) {
    TBERROR("Could not determine experiment leader!", 1);
}
TBUserInfo($expleader, $expleader_name, $expleader_email);

if (! TBProjLeader($pid, $projleader)) {
    TBERROR("Could not determine project leader!", 1);
}
if (! TBExptGroup($pid,$eid,$gid)) {
    TBERROR("Could not determine experiment group!", 1);
}
$leaders = TBLeaderMailList($pid,$gid);
if (! $leaders) {
    TBERROR("Could not get leader emails!", 1);
}

$lastact = TBGetExptLastAct($pid,$eid);
$idletime= TBGetExptIdleTime($pid,$eid,"G:i");
$idletime = str_replace(":0",":",$idletime);
list($idlehrs,$idlemin) = explode(":",$idletime);

$msg =
"Hi, this is an important automated message from $THISHOMEBASE.\n\n".
"It appears that the $c node".($c!=1?"s":"")." in your experiment ".
"'$eid' ".($c!=1?"have":"has")." been inactive for ".
"$idlehrs hours, $idlemin minutes, since $lastact. ".
( $swap_requests > 0 
  ? ("You have been sent ".$swap_requests." other message".
     ($swap_requests!=1?"s":"")." since this ".
     "experiment became idle. ")
  : "").
($swappable ?
 ("This experiment is marked as swappable, so it may be ".
  "automatically swapped out by $THISHOMEBASE or its ".
  "operational staff.\n") :
 ("This experiment has not been marked swappable, so it will not be ".
  "automatically swapped out.\n")).
"\nWe would appreciate it if you could either terminate or swap this ".
"experiment out so that the nodes will be available for use by ".
"other experimenters. You can do this by logging into the ".
"$THISHOMEBASE Web Interface, and using the swap or terminate ".
"links on this page:\n\n$TBBASE/showexp.php3?pid=$pid&eid=$eid\n\n".
"More information on experiment swapping is available in the $THISHOMEBASE ".
"FAQ at $TBDOCBASE/faq.php3#UTT-Swapping\n\n".
"More information on our node usage policy is available at ".
"$TBDOCBASE/docwrapper.php3?docname=swapping.html\n\n".
"If you feel this message is in error then please contact ".
"Testbed Operations <$TBMAILADDR_OPS>.\n\n".
"Thanks!\nTestbed Operations\n";

#print "<pre>\n$msg\n</pre>\n";

# For debugging:
#TBMAIL("Expt Leader <$TBMAILADDR_OPS>",
TBMAIL("$expleader_name <$expleader_email>",
       "$c PC".($c!=1?"s":"")." idle $idlehrs hours: $pid/$eid",
       wordwrap($msg,75),
       "From: $TBMAIL_OPS\n".
       ( $swap_requests >= $tell_proj_head
	 ? "Cc: $leaders\n"
	 : "") .
       "Bcc: $TBMAIL_OPS\n".
       "Errors-To: $TBMAIL_WWW");

#
# And log a copy so we know who sent it. 
#
TBUserInfo($uid, $uid_name, $uid_email);

TBMAIL($TBMAIL_AUDIT,
       "Swap or Terminate Request: $pid/$eid",
       "A swap/terminate $pid/$eid request was sent by $uid ($uid_name).\n",
       "From: $uid_name <$uid_email>\n".
       "X-NetBed: request_swapexp.php3\n".
       "Errors-To: $TBMAIL_WWW");

# Update the count and the time in the database
DBQueryWarn("update experiments set swap_requests= swap_requests+1,
             last_swap_req=now() where pid='$pid' and eid='$eid';");

echo "<p><center>
         Message successfully sent!
         </center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
