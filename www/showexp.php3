<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Only known and logged in users can look at experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

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
    DBQueryFatal("select e.idx,e.state,e.batchmode,e.linktest_pid,".
		 "       e.paniced,e.panic_date,s.rsrcidx,r.wirelesslans, ".
		 "       e.lockdown ".
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
$linktest_running = $row["linktest_pid"];
$paniced    = $row["paniced"];
$panic_date = $row["panic_date"];
$lockdown   = $row["lockdown"];

#
# Get a list of node types and classes in this experiment
#
$query_result =
    DBQueryFatal("select distinct v.type,t1.class,v.fixed,".
		 "   t2.type as ftype,t2.class as fclass from virt_nodes as v ".
		 "left join node_types as t1 on v.type=t1.type ".
		 "left join nodes as n on v.fixed is not null and ".
		 "     v.fixed=n.node_id ".
		 "left join node_types as t2 on t2.type=n.type ".
		 "where v.eid='$eid' and v.pid='$pid'");
while ($row = mysql_fetch_array($query_result)) {
    if (isset($row['ftype'])) {
	$classes[$row['fclass']] = 1;
	$types[$row['ftype']] = 1;
    }
    else {
	$classes[$row['class']] = 1;
	$types[$row['type']] = 1;
    }
}


echo "<font size=+2>Experiment <b>".
     "<a href='showproject.php3?pid=$pid'>$pid</a>/".
     "<a href='showexp.php3?pid=$pid&eid=$eid'>$eid</a></b></font>\n";
echo "<br /><br />\n";
SUBPAGESTART();

SUBMENUSTART("Experiment Options");

if ($expstate) {
    if (TBExptLogFile($exp_pid, $exp_eid)) {
	WRITESUBMENUBUTTON("View Activity Logfile",
			   "showlogfile.php3?pid=$exp_pid&eid=$exp_eid");
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

    WRITESUBMENUDIVIDER();

    if (!$lockdown) {
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
	    elseif ($expstate == $TB_EXPTSTATE_ACTIVE ||
		    ($expstate == $TB_EXPTSTATE_PANICED && $isadmin)) {
		WRITESUBMENUBUTTON("Swap Experiment Out",
			 "swapexp.php3?inout=out&pid=$exp_pid&eid=$exp_eid");
	    }
	    elseif ($expstate == $TB_EXPTSTATE_ACTIVATING) {
		WRITESUBMENUBUTTON("Cancel Experiment Swapin",
				   "swapexp.php3?inout=out".
				   "&pid=$exp_pid&eid=$exp_eid");
	    }
	}
    
	if ($expstate != $TB_EXPTSTATE_PANICED) {
	    WRITESUBMENUBUTTON("Terminate Experiment",
			       "endexp.php3?pid=$exp_pid&eid=$exp_eid");
	}

        # Batch experiments can be modifed only when paused.
	if ($expstate == $TB_EXPTSTATE_SWAPPED ||
	    (!$isbatch && $expstate == $TB_EXPTSTATE_ACTIVE)) {
	    WRITESUBMENUBUTTON("Modify Experiment",
			       "modifyexp.php3?pid=$exp_pid&eid=$exp_eid");
	}
    }
    
    if ($expstate == $TB_EXPTSTATE_ACTIVE) {
	WRITESUBMENUBUTTON("Modify Traffic Shaping",
			   "delaycontrol.php3?pid=$exp_pid&eid=$exp_eid");
    }
}

WRITESUBMENUBUTTON("Modify Metadata",
		   "editexp.php3?pid=$exp_pid&eid=$exp_eid");

WRITESUBMENUDIVIDER();

if ($expstate == $TB_EXPTSTATE_ACTIVE) {
    WRITESUBMENUBUTTON("Link Tracing/Monitoring",
		       "linkmon_list.php3?pid=$exp_pid&eid=$exp_eid");
    
    #
    # Admin and project/experiment leaders get this option.
    #
    if (TBExptAccessCheck($uid, $exp_pid, $exp_eid, $TB_EXPT_UPDATE)) {
	WRITESUBMENUBUTTON("Update All Nodes",
			   "updateaccounts.php3?pid=$exp_pid&eid=$exp_eid");
    }

    # Reboot option
    if (TBExptAccessCheck($uid, $exp_pid, $exp_eid, $TB_EXPT_MODIFY)) {
	WRITESUBMENUBUTTON("Reboot All Nodes",
			   "boot.php3?pid=$exp_pid&eid=$exp_eid");
    }
}

if (($expstate == $TB_EXPTSTATE_ACTIVE ||
     $expstate == $TB_EXPTSTATE_ACTIVATING ||
     $expstate == $TB_EXPTSTATE_MODIFY_RESWAP) &&
    (STUDLY() || $EXPOSELINKTEST)) {
    WRITESUBMENUBUTTON(($linktest_running ?
			"Stop LinkTest" : "Run LinkTest"), 
		       "linktest.php3?pid=$exp_pid&eid=$exp_eid".
		       ($linktest_running ? "&kill=1" : ""));
}

if ($expstate == $TB_EXPTSTATE_ACTIVE) {
    if (STUDLY() && $classes['pcvm']) {
	WRITESUBMENUBUTTON("Record Feedback Data",
			   "feedback.php3?pid=$exp_pid&".
			   "eid=$exp_eid&mode=record");
    }
}

if (($expstate == $TB_EXPTSTATE_ACTIVE ||
     $expstate == $TB_EXPTSTATE_SWAPPED) &&
    STUDLY()) {
    WRITESUBMENUBUTTON("Clear Feedback Data",
		       "feedback.php3?pid=$exp_pid&eid=$exp_eid&mode=clear");
    if ($classes['pcvm']) {
	    WRITESUBMENUBUTTON("Remap Virtual Nodes",
			       "remapexp.php3?pid=$exp_pid&eid=$exp_eid");
    }
}
    
# Wireless maps if experiment includes wireless lans.
if ($wireless) {
    WRITESUBMENUBUTTON("Wireless Node Map",
		       "floormap.php3".
		       ($expstate == $TB_EXPTSTATE_ACTIVE ?
			"?pid=$exp_pid&eid=$exp_eid" : ""));
}

WRITESUBMENUDIVIDER();

# History
WRITESUBMENUBUTTON("Show History",
		   "showstats.php3?showby=expt&which=$expindex");

if (STUDLY()) {
    WRITESUBMENUBUTTON("Duplicate Experiment",
		       "beginexp_html.php3?copyid=${exp_pid},${exp_eid}");
}

if ($EXPOSEARCHIVE) {
    WRITESUBMENUBUTTON("Experiment File Archive",
		       "archive_view.php3?pid=$exp_pid&eid=$exp_eid");
}

if ($types['garcia'] || $types['static-mica2'] || $types['robot']) {
    SUBMENUSECTION("Robot/Mote Options");
    WRITESUBMENUBUTTON("Robot/Mote Map",
		       "robotmap.php3".
		       ($expstate == $TB_EXPTSTATE_ACTIVE ?
			"?pid=$exp_pid&eid=$exp_eid" : ""));
    if ($expstate == $TB_EXPTSTATE_SWAPPED) {
	if ($types['static-mica2']) {
	    WRITESUBMENUBUTTON("Selector Applet",
			       "robotrack/selector.php3?".
			       "pid=$exp_pid&eid=$exp_eid");
	}
    }
    elseif ($expstate == $TB_EXPTSTATE_ACTIVE ||
	    $expstate == $TB_EXPTSTATE_ACTIVATING) {
	WRITESUBMENUBUTTON("Tracker Applet",
			   "robotrack/robotrack.php3?".
			   "pid=$exp_pid&eid=$exp_eid");
    }
}

# Blinky lights - but only if they have nodes of the correct type in their
# experiment
if ($classes['mote'] && $expstate == $TB_EXPTSTATE_ACTIVE) {
    WRITESUBMENUBUTTON("Show Blinky Lights",
		   "moteleds.php3?pid=$exp_pid&eid=$exp_eid","moteleds");
}

if ($isadmin) {
    if ($expstate == $TB_EXPTSTATE_ACTIVE) {
	SUBMENUSECTION("Beta-Test Options");
	WRITESUBMENUBUTTON("Restart Experiment",
			   "swapexp.php3?inout=restart&pid=$exp_pid".
			   "&eid=$exp_eid");
	WRITESUBMENUBUTTON("Replay Events",
			   "replayexp.php3?&pid=$exp_pid&eid=$exp_eid");

	SUBMENUSECTION("Admin Options");
	
	WRITESUBMENUBUTTON("Send an Idle Info Request",
			   "request_idleinfo.php3?".
			   "&pid=$exp_pid&eid=$exp_eid");
	
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
              src='showthumb.php3?idx=$rsrcidx'></a>";

SUBMENUEND_2B();

SHOWEXP($exp_pid, $exp_eid);

if (TBExptFirewall($exp_pid, $exp_eid) &&
    ($expstate == $TB_EXPTSTATE_ACTIVE ||
     $expstate == $TB_EXPTSTATE_PANICED ||
     $expstate == $TB_EXPTSTATE_ACTIVATING ||
     $expstate == $TB_EXPTSTATE_SWAPPING)) {
    echo "<center>\n";
    if ($paniced == 2) {
	#
	# Paniced due to failed swapout.
	# Only be semi-obnoxious (no blinking) since it wasn't their fault.
	#
	echo "<br><font size=+1 color=red>".
	     "Your experiment was cut off due to a failed swapout on $panic_date!".
	     "<br>".
	     "You will need to contact testbed operations to make further ".
  	     "changes (swap, terminate) to your experiment.</font>";
    }
    elseif ($paniced) {
	#
	# Paniced due to panic button.
  	# Full-on obnoxious is called for here!
	#
	echo "<br><font size=+1 color=red><blink>".
	     "Your experiment was cut off via the Panic Button on $panic_date!".
	     "<br>".
	     "You will need to contact testbed operations to make further ".
  	     "changes (swap, terminate) to your experiment.</blink></font>";
    }
    else {
	echo "<br><a href='panicbutton.php3?pid=$exp_pid&eid=$exp_eid'>
                 <img border=1 alt='panic button' src='panicbutton.gif'></a>";
	echo "<br><font color=red size=+2>".
	     " Press the Panic Button to contain your experiment".
	     "</font>\n";
    }
    echo "</center>\n";
}
SUBPAGEEND();

#
# Dump the node information.
#
SHOWNODES($exp_pid, $exp_eid, $sortby, $showclass);

if ($isadmin) {
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
