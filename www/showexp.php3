<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#
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

$expindex = TBExptIndex($exp_pid, $exp_eid);
$expstate = TBExptState($exp_pid, $exp_eid);

echo "<font size=+2>Experiment <b>".
     "<a href='showproject.php3?pid=$pid'>$pid</a>/".
     "$eid</b></font>\n";
echo "<br /><br />\n";
SUBPAGESTART();

SUBMENUSTART("Experiment Options");

if ($expstate) {
    if (TBExptLogFile($exp_pid, $exp_eid)) {
	WRITESUBMENUBUTTON("View Activation Logfile",
			   "spewlogfile.php3?pid=$exp_pid&eid=$exp_eid");
    }
      
    if (strcmp($expstate, $TB_EXPTSTATE_ACTIVE) == 0) {
	WRITESUBMENUBUTTON("Visualization, NS File, Mapping",
			   "shownsfile.php3?pid=$exp_pid&eid=$exp_eid");
    }
    elseif (strcmp($expstate, $TB_EXPTSTATE_SWAPPED) == 0) {
	WRITESUBMENUBUTTON("Visualization and NS File",
			   "shownsfile.php3?pid=$exp_pid&eid=$exp_eid");
    }
    else {
	WRITESUBMENUBUTTON("Visualization and NS File",
			   "shownsfile.php3?pid=$exp_pid&eid=$exp_eid");
    }

    # Swap option.
    if (strcmp($expstate, $TB_EXPTSTATE_SWAPPED) == 0) {
	WRITESUBMENUBUTTON("Swap this Experiment In",
		      "swapexp.php3?inout=in&pid=$exp_pid&eid=$exp_eid");
    }
    elseif (strcmp($expstate, $TB_EXPTSTATE_ACTIVE) == 0) {
	WRITESUBMENUBUTTON("Swap this Experiment Out",
		      "swapexp.php3?inout=out&pid=$exp_pid&eid=$exp_eid");

	WRITESUBMENUBUTTON("Control Traffic Shaping",
			   "delaycontrol.php3?pid=$exp_pid&eid=$exp_eid");
    }
}

WRITESUBMENUBUTTON("Terminate this Experiment",
		   "endexp.php3?pid=$exp_pid&eid=$exp_eid");

#
# Admin and project/experiment leader get this option.
#
if (TBExptAccessCheck($uid, $exp_pid, $exp_eid, $TB_EXPT_UPDATEACCOUNTS)) {
    WRITESUBMENUBUTTON("Update Mounts/Accounts",
		       "updateaccounts.php3?pid=$exp_pid&eid=$exp_eid");
}

# Reboot option
if (TBExptAccessCheck($uid, $exp_pid, $exp_eid, $TB_EXPT_MODIFY)) {
    WRITESUBMENUBUTTON("Reboot All Nodes",
		       "boot.php3?pid=$exp_pid&eid=$exp_eid");
    WRITESUBMENUBUTTON("Modify this Experiment",
		       "modifyexp.php3?pid=$exp_pid&eid=$exp_eid");

}

# History
WRITESUBMENUBUTTON("Show History",
		   "showstats.php3?showby=expt&which=$expindex");

if (ISADMIN($uid)) {
    if (strcmp($expstate, $TB_EXPTSTATE_ACTIVE) == 0) {		
	SUBMENUSECTION("Beta-Test Options");
	WRITESUBMENUBUTTON("Restart this Experiment",
			   "swapexp.php3?inout=restart&pid=$exp_pid".
			   "&eid=$exp_eid");

	SUBMENUSECTION("Admin Options");
	
	WRITESUBMENUBUTTON("Send a Swap Request",
			   "request_swapexp.php3?".
			   "&pid=$exp_pid&eid=$exp_eid");
	
	WRITESUBMENUBUTTON("Force Swap Out (Idle-Swap)",
			   "swapexp.php3?inout=out&force=1".
			   "&pid=$exp_pid&eid=$exp_eid");
	
	SUBMENUSECTIONEND();
    }
}
    
SUBMENUEND_2A();

echo "<br>
      <a href='shownsfile.php3?pid=$exp_pid&eid=$exp_eid'>
         <img width=160 height=160 border=1 alt='experiment vis'
              src='top2image.php3?pid=$pid&eid=$eid&thumb=160'></a>\n";

SUBMENUEND_2B();

#
# Dump experiment record.
# 
SHOWEXP($exp_pid, $exp_eid);

SUBPAGEEND();

#
# Dump the node information.
#
SHOWNODES($exp_pid, $exp_eid);

if (ISADMIN($uid)) {
    echo "<center>
          <h3>Experiment Stats</h3>
         </center>\n";

    SHOWEXPTSTATS($exp_pid, $exp_eid);
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
