<?php
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

if (!$confirmed) {
    echo "<center><h2><br>
          Are you sure you want to send an email message requesting that<br>
          experiment '$eid' be swapped or terminated?
          </h2>\n";

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
    TBERROR("Could not determine experiment leader!", 1);
}
TBUserInfo($projleader, $projleader_name, $projleader_email);

mail("$expleader_name <$expleader_email>",
     "TESTBED: Please Swap or Terminate Experiment: $pid/$eid",
     "Hi, this is an automated message from Emulab.Net.\n".
     "\n".
     "It appears that your experiment '$eid' in project '$pid' is inactive.\n".
     "We would appreciate it if you could either terminate or swap this\n".
     "this experiment out so that the nodes will be available for use by\n".
     "other experimenters. You can do this by logging into the Emulab Web\n".
     "Interface, and using the swap or terminate links on this page:\n".
     "\n".
     "        $TBBASE/showexp.php3?pid=$pid&eid=$eid\n".
     "\n".
     "More information on experiment swapping is available in the Emulab\n".
     "FAQ at http://www.emulab.net/faq.php3#UTT-Swapping\n".
     "\n".
     "If you feel this message is in error then please contact\n".
     "$TBMAILADDR_OPS.\n".
     "\n".
     "Thanks!\n".
     "Utah Network Testbed\n",
     "From: $TBMAIL_OPS\n".
     "Cc: $projleader_name <$projleader_email>\n".
     "Bcc: $TBMAIL_OPS\n".
     "Errors-To: $TBMAIL_WWW");

echo "<p><center>
         Message successfully sent!
         </center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
