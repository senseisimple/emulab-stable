<?php
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Show Experiment Information");

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
$exp_eid = $eid;
$exp_pid = $pid;

#
# Check to make sure this is a valid PID/EID tuple.
#
if (! TBValidExperiment($exp_pid, $exp_eid)) {
  USERERROR("The experiment $exp_eid is not a valid experiment ".
            "in project $exp_pid.", 1);
}

#
# Verify Permission.
#
if (! TBExptAccessCheck($uid, $exp_pid, $exp_eid, $TB_EXPT_READINFO)) {
    USERERROR("You do not have permission to view experiment $exp_eid!", 1);
}

#
# Dump experiment record.
# 
SHOWEXP($exp_pid, $exp_eid);

# Terminate option.
echo "<p><center>
       Do you want to terminate this experiment?
       <A href='endexp.php3?pid=$exp_pid&eid=$exp_eid'>Yes</a>
      </center>\n";    

# Swap option.
$expstate = TBExptState($exp_pid, $exp_eid);
if ($expstate) {
    if (strcmp($expstate, $TB_EXPTSTATE_SWAPPED) == 0) {
	echo "<p><center>
             Do you want to swap this experiment in?
             <A href='swapexp.php3?inout=in&pid=$exp_pid&eid=$exp_eid'>Yes</a>
             <br>
             <a href='$TBDOCBASE/faq.php3#UTT-Swapping'>
                (Information on experiment swapping)</a>
             </center>\n";
    }
    elseif (strcmp($expstate, $TB_EXPTSTATE_ACTIVE) == 0) {
	echo "<p><center>
             Do you want to swap this experiment out?
             <A href='swapexp.php3?inout=out&pid=$exp_pid&eid=$exp_eid'>Yes</a>
             <br>
             <a href='$TBDOCBASE/faq.php3#UTT-Swapping'>
                (Information on experiment swapping)</a>
             </center>\n";    
    }
}

#
# Dump the node information.
#
SHOWNODES($exp_pid, $exp_eid);

#
# Admin folks get a swap request link to send email.
#
if (ISADMIN($uid)) {
    echo "<p><center>
             <A href='request_swapexp.php3?&pid=$exp_pid&eid=$exp_eid'>
             Send a swap/terminate request</a>
             </center>\n";    
}
    
#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
