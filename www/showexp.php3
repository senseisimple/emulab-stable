<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
require("Sajax.php");
include("showstuff.php3");
sajax_init();
sajax_export("GetExpState", "Show");

#
# Only known and logged in users can look at experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);
$tag = "Experiment";

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
# For the Sajax Interface
#
function GetExpState($a, $b)
{
    global $pid, $eid;
    
    $expstate = TBExptState($pid, $eid);

    return $expstate;
}

function Show($which, $arg1, $arg2)
{
    global $pid, $eid, $uid, $TBSUEXEC_PATH, $TBADMINGROUP;
    $html = "";

    if ($which == "settings") {
	ob_start();
	SHOWEXP($pid, $eid);
	$html = ob_get_contents();
	ob_end_clean();
    }
    elseif ($which == "details") {
	$output = array();
	$retval = 0;
	$html   = "";

        # Show event summary and firewall info.
        $flags = "-b -e -f";

	$result = exec("$TBSUEXEC_PATH $uid $TBADMINGROUP ".
		       "webreport $flags $pid $eid",
		       $output, $retval);

	$html = "<pre><div align=left id=\"showexp_details\" ".
	    "class=\"showexp_codeblock\">";
	for ($i = 0; $i < count($output); $i++) {
	    $html .= htmlentities($output[$i]);
	    $html .= "\n";
	}
	$html .= "</div></pre>\n";

	$html .= "<button name=savedetails type=button value=1";
	$html .= " onclick=\"SaveDetails();\">";
	$html .= "Save</button>\n";
    }
    elseif ($which == "graphs") {
	$graphtype = $arg1;

	if (!isset($graphtype) || !$graphtype)
	    $graphtype = "pps";
	
	$exptidx = TBExptIndex($pid, $eid);
	# Make the link unique to force reload on the client side.
	$now     = time();
	
	$html  = "";
	$html .= "<div style='display: block; overflow: auto; ".
	         "     position: relative; height: 450px; ".
	         "     width: 90%; border: 2px solid black;'>\n";
	$html .= "  <img border=0 ";
	$html .= "       src='linkgraph_image.php?exptidx=$exptidx";
	$html .= "&graphtype=$graphtype&now=$now'>\n";
	$html .= "</div>\n";

	$html .= "<button name=pps type=button value=1";
	$html .= " onclick=\"GraphChange('pps');\">";
	$html .= "Packets</button>\n";
	$html .= "<button name=bps type=button value=1";
	$html .= " onclick=\"GraphChange('bps');\">";
	$html .= "Bytes</button>\n";
    }
    elseif ($which == "vis") {
	$zoom   = $arg1;
	$detail = $arg2;
	
	if ($zoom == 0) {
            # Default is whatever we have; to avoid regen of the image.
	    $query_result =
		DBQueryFatal("select zoom,detail from vis_graphs ".
			     "where pid='$pid' and eid='$eid'");

	    if (mysql_num_rows($query_result)) {
		$row    = mysql_fetch_array($query_result);
		$zoom   = $row['zoom'];
		$detail = $row['detail'];
	    }
	    else {
		$zoom   = 1.15;
		$detail = 1;
	    }
	}
	else {
            # Sanity check but lets not worry about throwing an error.
	    if (!TBvalid_float($zoom))
		$zoom = 1.25;
	    if (!TBvalid_integer($detail))
		$detail = 1;
    	}

	$html = ShowVis($pid, $eid, $zoom, $detail);

	$zoomout = sprintf("%.2f", $zoom / 1.25);
	$zoomin  = sprintf("%.2f", $zoom * 1.25);

	$html .= "<button name=viszoomout type=button value=$zoomout";
	$html .= " onclick=\"VisChange('$zoomout', $detail);\">";
	$html .= "Zoom Out</button>\n";
	$html .= "<button name=viszoomin type=button value=$zoomin";
	$html .= " onclick=\"VisChange('$zoomin', $detail);\">";
	$html .= "Zoom In</button>\n";

	if ($detail) {
	    $html .= "<button name=hidedetail type=button value=0";
	    $html .= " onclick=\"VisChange('$zoom', 0);\">";
	    $html .= "Hide Details</button>\n";
	}
	else {
	    $html .= "<button name=showdetail type=button value=1";
	    $html .= " onclick=\"VisChange('$zoom', 1);\">";
	    $html .= "Show Details</button>\n";
	}
	$html .= "&nbsp &nbsp &nbsp &nbsp &nbsp &nbsp ";
	$html .= "<button name=fullscreenvis type=button value=1";
	$html .= " onclick=\"FullScreenVis();\">";
	$html .= "Full Screen</button>\n";
    }
    elseif ($which == "nsfile") {
	$nsdata = "";
	
	$query_result =
	    DBQueryFatal("select nsfile from nsfiles ".
			 "where pid='$pid' and eid='$eid'");
	if (mysql_num_rows($query_result)) {
	    $row    = mysql_fetch_array($query_result);
	    $nsdata = htmlentities($row["nsfile"]);
	}
	$html = "<pre><div align=left class=\"showexp_codeblock\">".
	    "$nsdata</div></pre>\n";

	$html .= "<button name=savens type=button value=1";
	$html .= " onclick=\"SaveNS();\">";
	$html .= "Save</button>\n";
    }
    return $html;
}

#
# Dump the visualization into its own iframe.
#
function ShowVis($pid, $eid, $zoom = 1.25, $detail = 1) {
    $html = "<div id=fee style='display: block; overflow: hidden; ".
	    "     position: relative; z-index:1010; height: 450px; ".
	    "     width: 90%; border: 2px solid black;'>\n".
            " <div id=myvisdiv style='position:relative;'>\n".
	    "   <img id=myvisimg border=0 ".
	    "        onLoad=\"setTimeout('ShowVisInit();', 10);\" ".
	    "        src='top2image.php3?pid=$pid&eid=$eid".
	    "&zoom=$zoom&detail=$detail'>\n".
	    " </div>\n".
	    "</div>\n";

    return $html;
}

#
# See if this request is to the above function. Does not return
# if it is. Otherwise return and continue on.
#
sajax_handle_client_request();

# Faster to do this after the sajax stuff
include_once("template_defs.php");

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

# Template Instance Experiments get special treatment in this page.
$instance = NULL;
if ($EXPOSETEMPLATES) {
     $instance = TemplateInstance::LookupByExptidx($expindex);

     if (! is_null($instance)) {
	 $tag = "Template Instance";
	 $guid = $instance->guid();
	 $vers = $instance->vers();
     }
}

#
# Standard Testbed Header.
#
PAGEHEADER("$tag ($pid/$eid)");

echo "<script type='text/javascript' src='showexp.js'></script>\n";
#
# This has to happen ...
#
$bodyclosestring = "<script type='text/javascript'>SET_DHTML();</script>\n";

echo "<script type='text/javascript' src='js/wz_dragdrop.js'></script>";
echo "<script type='text/javascript' language='javascript'>\n";
sajax_show_javascript();
echo "StartStateChangeWatch('$pid', '$eid', '$expstate');\n";
echo "</script>\n";

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

echo "<font size=+2>$tag <b>".
     "<a href='showproject.php3?pid=$pid'>$pid</a>/".
     "<a href='showexp.php3?pid=$pid&eid=$eid'>$eid</a></b></font>\n";
echo "<br /><br />\n";
SUBPAGESTART();

SUBMENUSTART("$tag Options");

if ($expstate) {
    if (TBExptLogFile($exp_pid, $exp_eid)) {
	WRITESUBMENUBUTTON("View Activity Logfile",
			   "showlogfile.php3?pid=$exp_pid&eid=$exp_eid");
    }
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
		WRITESUBMENUBUTTON(($instance ?
				    "Terminate Instance" :
				    "Swap Experiment Out"),
			 "swapexp.php3?inout=out&pid=$exp_pid&eid=$exp_eid");
	    }
	    elseif ($expstate == $TB_EXPTSTATE_ACTIVATING) {
		WRITESUBMENUBUTTON("Cancel Experiment Swapin",
				   "swapexp.php3?inout=out".
				   "&pid=$exp_pid&eid=$exp_eid");
	    }
	}
    
	if (!$instance && $expstate != $TB_EXPTSTATE_PANICED) {
	    WRITESUBMENUBUTTON("Terminate Experiment",
			       "endexp.php3?pid=$exp_pid&eid=$exp_eid");
	}

        # Batch experiments can be modifed only when paused.
	if (!$instance && ($expstate == $TB_EXPTSTATE_SWAPPED ||
	    (!$isbatch && $expstate == $TB_EXPTSTATE_ACTIVE))) {
	    WRITESUBMENUBUTTON("Modify Experiment",
			       "modifyexp.php3?pid=$exp_pid&eid=$exp_eid");
	}
    }

    if ($instance && $expstate == $TB_EXPTSTATE_ACTIVE) {
	WRITESUBMENUBUTTON("Start New Experiment Run",
			   "template_exprun.php?action=start&guid=$guid".
			   "&version=$vers&eid=$exp_eid");

	WRITESUBMENUBUTTON("Create New Template",
			   "template_commit.php?&guid=$guid".
			   "&version=$vers&exptidx=$expindex");
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

if (!$instance && STUDLY()) {
    WRITESUBMENUBUTTON("Duplicate Experiment",
		       "beginexp_html.php3?copyid=${exp_pid},${exp_eid}");
}

if ($EXPOSEARCHIVE) {
    WRITESUBMENUBUTTON("Experiment File Archive",
		       "archive_view.php3?exptidx=$expindex");
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

#
# The center area is a form that can show NS file, Details, or Vis.
# IE complicates this, although in retrospect, I could have used plain
# input buttons instead of the fancy rendering kind of buttons, which do not
# work as expected (violates the html spec) in IE. 
#
echo "<script type='text/javascript' language='javascript'>
        var li_current = 'li_settings';
        function Show(which) {
	    li = getObjbyName(li_current);
            li.style.backgroundColor = '#DDE';
            li.style.borderBottom = 'none';

            li_current = 'li_' + which;
	    li = getObjbyName(li_current);
            li.style.backgroundColor = 'white';
            li.style.borderBottom = '1px solid white';

            x_Show(which, 0, 0, Show_cb);
            return false;
        }
        function Show_cb(html) {
	    visarea = getObjbyName('showexp_visarea');
            if (visarea) {
                visarea.innerHTML = html;
            }
        }
        function ShowVisInit() {
            ADD_DHTML(\"myvisdiv\");
        }
        function VisChange(zoom, detail) {
            x_Show('vis', zoom, detail, Show_cb);
            return false;
        }
        function GraphChange(which) {
            x_Show('graphs', which, 0, Show_cb);
            return false;
        }
        function SaveDetails() {
            window.open('spitreport.php?pid=$pid&eid=$eid',
                        '_blank','width=700,height=400,toolbar=no,".
                        "resizeable=yes,scrollbars=yes,status=yes,".
	                "menubar=yes');
        }
        function SaveNS() {
            window.open('spitnsdata.php3?pid=$pid&eid=$eid',
                        '_blank','width=700,height=400,toolbar=no,".
                        "resizeable=yes,scrollbars=yes,status=yes,".
	                "menubar=yes');
        }
        function FullScreenVis() {
	    window.location.replace('shownsfile.php3?pid=$pid&eid=$eid');
        }
      </script>\n";

#
# This is the topbar
#
echo "<div width=\"100%\" align=center>\n";
echo "<ul id=\"topnavbar\">\n";
echo "<li>
          <a href=\"#A\" style=\"background-color:white\" ".
               "id=\"li_settings\" onclick=\"Show('settings');\">".
               "Settings</a></li>\n";
echo "<li>
          <a href=\"#B\" id=\"li_vis\" onclick=\"Show('vis');\">".
               "Visualization</a></li>\n";
echo "<li>
          <a href=\"#C\" id=\"li_nsfile\" onclick=\"Show('nsfile');\">".
              "NS File</a></li>\n";
echo "<li>
          <a href=\"#D\" id=\"li_details\" onclick=\"Show('details');\">".
              "Details</a></li>\n";

if ($instance && $expstate == $TB_EXPTSTATE_ACTIVE) {
    echo "<li>
              <a href=\"#E\" id=\"li_graphs\" onclick=\"Show('graphs');\">".
                  "Graphs</a></li>\n";
}
echo "</ul>\n";

#
# Start out with details ...
#
echo "<div align=center width=\"100%\" id=\"showexp_visarea\">\n";
SHOWEXP($exp_pid, $exp_eid);
echo "</div>\n";
echo "</div>\n";

if (TBExptFirewall($exp_pid, $exp_eid) &&
    ($expstate == $TB_EXPTSTATE_ACTIVE ||
     $expstate == $TB_EXPTSTATE_PANICED ||
     $expstate == $TB_EXPTSTATE_ACTIVATING ||
     $expstate == $TB_EXPTSTATE_SWAPPING)) {
    echo "<center>\n";
    if ($paniced == 2) {
	#
	# Paniced due to failed swapout.
	# Only be semi-obnoxious (no blinking) since it was not their fault.
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

if ($instance &&
    ($expstate == $TB_EXPTSTATE_ACTIVE ||
     $expstate == $TB_EXPTSTATE_PANICED ||
     $expstate == $TB_EXPTSTATE_ACTIVATING)) {
    $instance->ShowBindings();
}

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
