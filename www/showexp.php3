<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2004 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Only known and logged in users can look at experiments.
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
if (!isset($sortby)) {
    if ($pid == $TBOPSPID)
	$sortby = "rsrvtime-down";
    else
	$sortby = "";
}
$exp_eid = $eid;
$exp_pid = $pid;

#
# Standard Testbed Header now that we have the pid/eid okay.
#
PAGEHEADER("Experiment Information ($pid/$eid)");

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
# Need some DB info.
#
$query_result =
    DBQueryFatal("select e.idx,e.state,e.batchmode,s.rsrcidx,r.wirelesslans ".
		 "  from experiments as e ".
		 "left join experiment_stats as s on s.exptidx=e.idx ".
		 "left join experiment_resources as r on s.rsrcidx=r.idx ".
		 "where e.eid='$eid' and e.pid='$pid'");
$row        = mysql_fetch_array($query_result);
$expindex   = $row["idx"];
$expstate   = $row["state"];
$rsrcidx    = $row["rsrcidx"];
$isbatch    = $row["batchmode"];
$wireless   = $row["wirelesslans"];

echo "<font size=+2>Experiment <b>".
     "<a href='showproject.php3?pid=$pid'>$pid</a>/".
     "<a href='showexp.php3?pid=$pid&eid=$eid'>$eid</a></b></font>\n";
echo "<br /><br />\n";
SUBPAGESTART();

SUBMENUSTART("Experiment Options");

if ($expstate) {
    if (TBExptLogFile($exp_pid, $exp_eid)) {
	WRITESUBMENUBUTTON("View Activity Logfile",
			   "spewlogfile.php3?pid=$exp_pid&eid=$exp_eid");
    }
      
    if ($state == $TB_EXPTSTATE_ACTIVE) {
	WRITESUBMENUBUTTON("Visualization, NS File, Mapping",
			   "shownsfile.php3?pid=$exp_pid&eid=$exp_eid");
    }
    else {
	WRITESUBMENUBUTTON("Visualization and NS File",
			   "shownsfile.php3?pid=$exp_pid&eid=$exp_eid");
    }
    WRITESUBMENUBUTTON("Download NS File",
		       "spitnsdata.php3?pid=$exp_pid&eid=$exp_eid");

    # Swap option.
    if ($isbatch) {
	if ($expstate == $TB_EXPTSTATE_SWAPPED) {
	    WRITESUBMENUBUTTON("Queue Batch Experiment",
			"swapexp.php3?inout=in&pid=$exp_pid&eid=$exp_eid");
	}
	elseif ($expstate == $TB_EXPTSTATE_ACTIVE ||
		$expstate == $TB_EXPTSTATE_ACTIVATING) {
	    WRITESUBMENUBUTTON("Stop Batch Experiment",
			"swapexp.php3?inout=out&pid=$exp_pid&eid=$exp_eid");
	}
	elseif ($expstate == $TB_EXPTSTATE_QUEUED) {
	    WRITESUBMENUBUTTON("Dequeue Batch Experiment",
			"swapexp.php3?inout=pause&pid=$exp_pid&eid=$exp_eid");
	}
    }
    else {
	if ($expstate == $TB_EXPTSTATE_SWAPPED) {
	    WRITESUBMENUBUTTON("Swap Experiment In",
			"swapexp.php3?inout=in&pid=$exp_pid&eid=$exp_eid");
	}
	elseif ($expstate == $TB_EXPTSTATE_ACTIVE) {
	    WRITESUBMENUBUTTON("Swap Experiment Out",
			"swapexp.php3?inout=out&pid=$exp_pid&eid=$exp_eid");
	}
	elseif ($expstate == $TB_EXPTSTATE_ACTIVATING) {
	    WRITESUBMENUBUTTON("Cancel Experiment Swapin",
			       "swapexp.php3?inout=out".
			       "&pid=$exp_pid&eid=$exp_eid");
	}
    }
    WRITESUBMENUBUTTON("Terminate Experiment",
		       "endexp.php3?pid=$exp_pid&eid=$exp_eid");

    # Batch experiments can be modifed only when paused.
    if ($expstate == $TB_EXPTSTATE_SWAPPED ||
	(!$isbatch && $expstate == $TB_EXPTSTATE_ACTIVE)) {
	WRITESUBMENUBUTTON("Modify Experiment",
			   "modifyexp.php3?pid=$exp_pid&eid=$exp_eid");
    }
    
    if ($expstate == $TB_EXPTSTATE_ACTIVE) {
	WRITESUBMENUBUTTON("Modify Traffic Shaping",
			   "delaycontrol.php3?pid=$exp_pid&eid=$exp_eid");
    }
}

WRITESUBMENUBUTTON("Edit Experiment Metadata",
		   "editexp.php3?pid=$exp_pid&eid=$exp_eid");

#
# Admin and project/experiment leader get this option.
#
if ($expstate == $TB_EXPTSTATE_ACTIVE &&
    TBExptAccessCheck($uid, $exp_pid, $exp_eid, $TB_EXPT_UPDATE)) {
    WRITESUBMENUBUTTON("Update All Nodes",
		       "updateaccounts.php3?pid=$exp_pid&eid=$exp_eid");
}

# Reboot option
if (TBExptAccessCheck($uid, $exp_pid, $exp_eid, $TB_EXPT_MODIFY)) {
    WRITESUBMENUBUTTON("Reboot All Nodes",
		       "boot.php3?pid=$exp_pid&eid=$exp_eid");
}

# Wireless maps if experiment includes wireless lans.
if ($wireless) {
    WRITESUBMENUBUTTON("Wireless Node Map",
		       "floormap.php3".
		       ($expstate == $TB_EXPTSTATE_ACTIVE ?
			"?pid=$exp_pid&eid=$exp_eid" : ""));
}

# History
WRITESUBMENUBUTTON("Show History",
		   "showstats.php3?showby=expt&which=$expindex");

if (STUDLY()) {
    WRITESUBMENUBUTTON("Run Linktest",
		       "linktest.php3?pid=$exp_pid&eid=$exp_eid");
}

if (ISADMIN($uid)) {
    if ($expstate == $TB_EXPTSTATE_ACTIVE) {
	SUBMENUSECTION("Beta-Test Options");
	WRITESUBMENUBUTTON("Restart Experiment",
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
         <img border=1 alt='experiment vis'
              src='showthumb.php3?idx=$rsrcidx'></a>\n";

SUBMENUEND_2B();

SHOWEXP($exp_pid, $exp_eid);

SUBPAGEEND();

#
# Dump the node information.
#
SHOWNODES($exp_pid, $exp_eid, $sortby);

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
