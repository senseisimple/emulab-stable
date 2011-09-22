<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2011 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
require("Sajax.php");
include("showstuff.php3");
include_once("node_defs.php");
include_once("template_defs.php");
sajax_init();
sajax_export("GetExpState", "Show", "ModifyAnno", "FreeNodeHtml");

#
# Only known and logged in users can look at experiments.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$reqargs = RequiredPageArguments("experiment", PAGEARG_EXPERIMENT);
$optargs = OptionalPageArguments("sortby",     PAGEARG_STRING,
				 "showclass",  PAGEARG_STRING);

if (!isset($sortby)) {
    if ($experiment->pid() == $TBOPSPID)
	$sortby = "rsrvtime-down";
    else
	$sortby = "";
}
if (!isset($showclass))
     $showclass = null;

# Need these below.
$exp_eid = $eid = $experiment->eid();
$exp_pid = $pid = $experiment->pid();
$tag = "Experiment";

#
# Verify Permission.
#
if (!$experiment->AccessCheck($this_user, $TB_EXPT_READINFO)) {
    USERERROR("You do not have permission to view experiment $exp_eid!", 1);
}

# Template Instance Experiments get special treatment in this page.
$instance = NULL;
if ($EXPOSETEMPLATES) {
     $instance = TemplateInstance::LookupByExptidx($experiment->idx());

     if (! is_null($instance)) {
	 $tag = "Instance";
	 $guid = $instance->guid();
	 $vers = $instance->vers();
     }
}

#
# For the Sajax Interface
#
$USER_VIS_URL = "http://$USERNODE/exp-vis/$pid/$eid/";
$HAVE_USER_VIS = 0;
$whocares = null;
if ($EXP_VIS && CHECKURL($USER_VIS_URL, $whocares)) {
  $HAVE_USER_VIS = 1;
}

function FreeNodeHtml()
{
    global $this_user, $experiment;
    
    return ShowFreeNodes($this_user, $experiment->Group());
}

function GetExpState($a, $b)
{
    global $experiment;

    return $experiment->state();
}

function ModifyAnno($newtext)
{
    global $this_user, $instance;

    $instance->SetAnnotation($this_user, $newtext);
    return 0;
}

function Show($which, $arg1, $arg2)
{
    global $experiment, $instance, $uid, $TBSUEXEC_PATH, $TBADMINGROUP;
    global $USER_VIS_URL;
    $pid  = $experiment->pid();
    $eid  = $experiment->eid();
    $html = "";

    if ($which == "settings") {
	ob_start();
	$experiment->Show();
	$html = ob_get_contents();
	ob_end_clean();
    }
    if ($which == "anno") {
	if (isset($instance)) {
	    ob_start();
	    $instance->ShowAnnotation(1);
	    $html = ob_get_contents();
	    ob_end_clean();
	}
    }
    elseif ($which == "details") {
	$showevents = $arg1;
	$output = array();
	$retval = 0;
	$html   = "";

        # Show event summary and firewall info.
        $flags = ($showevents ? "-e -a" : "-b -e -f");

	$result = exec("$TBSUEXEC_PATH $uid $TBADMINGROUP ".
		       "webtbreport $flags $pid $eid",
		       $output, $retval);

	$html = "<pre><div align=left id=\"showexp_details\" ".
	    "class=\"showexp_codeblock\">";
	for ($i = 0; $i < count($output); $i++) {
	    $html .= htmlentities($output[$i]);
	    $html .= "\n";
	}
	$html .= "</div></pre>\n";

	$html .= "<button name=showevents type=button value=1";
	$html .= " onclick=\"ShowEvents();\">";
	$html .= "Show Events</button>\n";
	
	$html .= "&nbsp &nbsp &nbsp &nbsp &nbsp &nbsp ";

	$html .= "<button name=savedetails type=button value=1";
	$html .= " onclick=\"SaveDetails();\">";
	$html .= "Save to File</button>\n";
    }
    elseif ($which == "graphs") {
	$graphtype = $arg1;

	if (!isset($graphtype) || !$graphtype)
	    $graphtype = "pps";
	
	$exptidx = $experiment->idx();
	# Make the link unique to force reload on the client side.
	$now     = time();
	
	$html  = "";
	$html .= "<div style='display: block; overflow: auto; ".
	         "     position: relative; height: 450px; ".
	         "     width: 90%; border: 2px solid black;'>\n";
	$html .= "  <img border=0 ";
	$html .= "       src='linkgraph_image.php?instance=$exptidx";
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
	$nsdata = $experiment->NSFile();
	
	$html = "<pre><div align=left class=\"showexp_codeblock\">".
	    "$nsdata</div></pre>\n";

	$html .= "<button name=savens type=button value=1";
	$html .= " onclick=\"SaveNS();\">";
	$html .= "Save</button>\n";
    }
    elseif ($which == "uservis") {
	ob_start();
	$html .= "<iframe src=\"$USER_VIS_URL\" width=\"100%\" height=600 id=\"vis-iframe\"></iframe>";
	ob_end_clean();
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
	    "   <img id=myvisimg border=0 style='cursor: move;' ".
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
$expindex   = $experiment->idx();
$expstate   = $experiment->state();
$isbatch    = $experiment->batchmode();
$linktest_running = $experiment->linktest_pid();
$paniced    = $experiment->paniced();
$panic_date = $experiment->panic_date();
$lockdown   = $experiment->lockdown();
$geniflags  = $experiment->geniflags();

if (! ($experiment_stats = $experiment->GetStats())) {
    TBERROR("Could not get experiment stats object for $expindex", 1);
}
$rsrcidx    = $experiment_stats->rsrcidx();
if (! ($experiment_resources = $experiment->GetResources())) {
    TBERROR("Could not get experiment resources object for $expindex", 1);
}
$wireless   = $experiment_resources->wirelesslans();

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

SUBPAGESTART();

SUBMENUSTART("$tag Options");

if ($expstate) {
    if ($experiment->logfile() && $experiment->logfile() != "") {
	WRITESUBMENUBUTTON("View Activity Logfile",
			   CreateURL("showlogfile", $experiment));
    }
    WRITESUBMENUDIVIDER();

    if (!$lockdown) {
        # Swap option.
	if ($isbatch) {
	    if ($expstate == $TB_EXPTSTATE_SWAPPED) {
		WRITESUBMENUBUTTON("Queue Batch Experiment",
				   CreateURL("swapexp", $experiment,
					     "inout", "in"));
	    }
	    elseif ($expstate == $TB_EXPTSTATE_ACTIVE ||
		    $expstate == $TB_EXPTSTATE_ACTIVATING) {
		WRITESUBMENUBUTTON("Stop Batch Experiment",
				   CreateURL("swapexp", $experiment,
					     "inout", "out"));
	    }
	    elseif ($expstate == $TB_EXPTSTATE_QUEUED) {
		WRITESUBMENUBUTTON("Dequeue Batch Experiment",
				   CreateURL("swapexp", $experiment,
					     "inout", "pause"));
	    }
	}
	else {
	    if (!$geniflags && $expstate == $TB_EXPTSTATE_SWAPPED) {
		WRITESUBMENUBUTTON(($instance ?
				    "Swap Instance In" :
				    "Swap Experiment In"),
				   CreateURL("swapexp", $experiment,
					     "inout", "in"));
	    }
	    elseif ($expstate == $TB_EXPTSTATE_ACTIVE ||
		    ($expstate == $TB_EXPTSTATE_PANICED && $isadmin)) {
		WRITESUBMENUBUTTON(($instance ?
				    "Terminate Instance" :
				    "Swap Experiment Out"),
				   CreateURL("swapexp", $experiment,
					     "inout", "out"));
	    }
	    elseif (!$geniflags && $expstate == $TB_EXPTSTATE_ACTIVATING) {
		WRITESUBMENUBUTTON(($instance ?
				   "Cancel Template Instantiation" :
  				   "Cancel Experiment Swapin"),
				   CreateURL("swapexp", $experiment,
					     "inout", "out"));
	    }
	}
    
	if (!$instance && !$geniflags && $expstate != $TB_EXPTSTATE_PANICED) {
	    WRITESUBMENUBUTTON("Terminate Experiment",
			       CreateURL("endexp", $experiment));
	}
	elseif ($instance && $expstate == $TB_EXPTSTATE_SWAPPED) {
	    WRITESUBMENUBUTTON("Terminate Instance",
			       CreateURL("endexp", $experiment));
	}

        # Batch experiments can be modifed only when paused.
	if (!$geniflags &&
	    !$instance && ($expstate == $TB_EXPTSTATE_SWAPPED ||
	    (!$isbatch && $expstate == $TB_EXPTSTATE_ACTIVE))) {
	    WRITESUBMENUBUTTON("Modify Experiment",
			       CreateURL("modifyexp", $experiment));
	}
    }

    if ($instance && $expstate == $TB_EXPTSTATE_ACTIVE) {
	if ($instance->runidx()) {
	    WRITESUBMENUBUTTON("Stop Current Run",
			       CreateURL("template_exprun", $instance,
					 "action", "stop"));
	    WRITESUBMENUBUTTON("Abort Current Run",
			       CreateURL("template_exprun", $instance,
					 "action", "abort"));
	}
	#WRITESUBMENUBUTTON("Modify Resources",
	#		   CreateURL("template_exprun", $instance,
	#			     "action", "modify"));

	WRITESUBMENUBUTTON("Start New Run",
			   CreateURL("template_exprun", $instance,
				     "action", "start"));

	if ($instance->pause_time()) {
	    WRITESUBMENUBUTTON("Continue Experiment RunTime",
			       CreateURL("template_exprun", $instance,
					 "action", "continue"));
	}
	else {
	    WRITESUBMENUBUTTON("Pause Runtime",
			       CreateURL("template_exprun", $instance,
					 "action", "pause"));
	}

	WRITESUBMENUBUTTON("Create New Template",
			   CreateURL("template_commit", $instance));
    }
    
    if (!$geniflags && $expstate == $TB_EXPTSTATE_ACTIVE) {
	WRITESUBMENUBUTTON("Modify Traffic Shaping",
			   CreateURL("delaycontrol", $experiment));
    }
}

WRITESUBMENUBUTTON("Modify Settings",
		   CreateURL("editexp", $experiment));

WRITESUBMENUDIVIDER();

if ($expstate == $TB_EXPTSTATE_ACTIVE) {
    if (!$geniflags) {
	WRITESUBMENUBUTTON("Link Tracing/Monitoring",
			   CreateURL("linkmon_list", $experiment));
    
	WRITESUBMENUBUTTON("Event Viewer",
			   CreateURL("showevents", $experiment));
    }
    
    #
    # Admin and project/experiment leaders get this option.
    #
    if ($experiment->AccessCheck($this_user, $TB_EXPT_UPDATE)) {
	WRITESUBMENUBUTTON("Update All Nodes",
			   CreateURL("updateaccounts", $experiment));
    }

    # Reboot option
    if ($experiment->AccessCheck($this_user, $TB_EXPT_MODIFY)) {
	WRITESUBMENUBUTTON("Reboot All Nodes",
			   CreateURL("boot", $experiment));
    }
}

if (($expstate == $TB_EXPTSTATE_ACTIVE ||
     $expstate == $TB_EXPTSTATE_ACTIVATING ||
     $expstate == $TB_EXPTSTATE_MODIFY_RESWAP) &&
    (STUDLY() || $EXPOSELINKTEST)) {
    WRITESUBMENUBUTTON(($linktest_running ?
			"Stop LinkTest" : "Run LinkTest"),
		       CreateURL("linktest", $experiment) . 
		       ($linktest_running ? "&kill=1" : ""));
}

if ($expstate == $TB_EXPTSTATE_ACTIVE) {
    if (!$geniflags && STUDLY() && isset($classes['pcvm'])) {
	WRITESUBMENUBUTTON("Record Feedback Data",
			   CreateURL("feedback", $experiment) .
			   "&mode=record");
    }
}

if (($expstate == $TB_EXPTSTATE_ACTIVE ||
     $expstate == $TB_EXPTSTATE_SWAPPED) &&
    !$geniflags && STUDLY()) {
    WRITESUBMENUBUTTON("Clear Feedback Data",
		       CreateURL("feedback", $experiment) . "&mode=clear");
    if (isset($classes['pcvm'])) {
	    WRITESUBMENUBUTTON("Remap Virtual Nodes",
			       CreateURL("remapexp", $experiment));
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
if (! $instance) {
    WRITESUBMENUBUTTON("Show History",
		       "showstats.php3?showby=expt&exptidx=$expindex");
}

if (!$geniflags) {
    WRITESUBMENUBUTTON("Duplicate Experiment",
		       "beginexp_html.php3?copyid=$expindex");
}

if ($EXPOSEARCHIVE && !$instance && !$geniflags) {
    WRITESUBMENUBUTTON("Experiment File Archive",
		       "archive_view.php3?experiment=$expindex");
}

if (isset($types['garcia']) ||
    isset($types['static-mica2']) ||
    isset($types['robot'])) {
    SUBMENUSECTION("Robot/Mote Options");
    WRITESUBMENUBUTTON("Robot/Mote Map",
		       "robotmap.php3".
		       ($expstate == $TB_EXPTSTATE_ACTIVE ?
			"?pid=$exp_pid&eid=$exp_eid" : ""));
    if ($expstate == $TB_EXPTSTATE_SWAPPED) {
	if (isset($types['static-mica2']) && $types['static-mica2']) {
	    WRITESUBMENUBUTTON("Selector Applet",
			       "robotrack/selector.php3?".
			       "pid=$exp_pid&eid=$exp_eid");
	}
    }
    elseif ($expstate == $TB_EXPTSTATE_ACTIVE ||
	    $expstate == $TB_EXPTSTATE_ACTIVATING) {
	WRITESUBMENUBUTTON("Tracker Applet",
			   CreateURL("robotrack/robotrack", $experiment));
    }
}

# Blinky lights - but only if they have nodes of the correct type in their
# experiment
if (isset($classes['mote']) && $expstate == $TB_EXPTSTATE_ACTIVE) {
    WRITESUBMENUBUTTON("Show Blinky Lights",
		       CreateURL("moteleds", $experiment), "moteleds");
}

if ($isadmin) {
    if ($expstate == $TB_EXPTSTATE_ACTIVE ||
	$expstate == $TB_EXPTSTATE_PANICED) {

	if ($expstate == $TB_EXPTSTATE_ACTIVE && !$geniflags) {
	    SUBMENUSECTION("Beta-Test Options");
	    WRITESUBMENUBUTTON("Restart Experiment",
			       CreateURL("swapexp", $experiment,
					 "inout", "restart"));
	    WRITESUBMENUBUTTON("Replay Events",
			       CreateURL("replayexp", $experiment));
	}

	SUBMENUSECTION("Admin Options");

	if ($expstate == $TB_EXPTSTATE_ACTIVE && !$geniflags) {
	    WRITESUBMENUBUTTON("Send an Idle Info Request",
			       CreateURL("request_idleinfo", $experiment));
	
	    WRITESUBMENUBUTTON("Send a Swap Request",
			       CreateURL("request_swapexp", $experiment));
	}
	if ($expstate == $TB_EXPTSTATE_PANICED) {
	    WRITESUBMENUBUTTON("Clear Panic Mode",
			       CreateURL("panicbutton", $experiment,
					 "clear", 1));
	}
	else {
	    WRITESUBMENUBUTTON("Panic Mode (level 1)",
			       CreateURL("panicbutton", $experiment,
					 "level", 1));
	    WRITESUBMENUBUTTON("Panic Mode (level 2)",
			       CreateURL("panicbutton", $experiment,
					 "level", 2));

	    WRITESUBMENUBUTTON("Force Swap Out (Idle-Swap)",
			       CreateURL("swapexp", $experiment,
					 "inout", "out", "force", 1));
	}
	SUBMENUSECTIONEND();
    }
}
    
SUBMENUEND_2A();

echo "<br>\n";
echo "<script>\n";
echo "function FreeNodeHtml_CB(stuff) {
         getObjbyName('showexpusagefreenodes').innerHTML = stuff;
         setTimeout('GetFreeNodeHtml()', 60000);
      }
      function GetFreeNodeHtml() {
         x_FreeNodeHtml(FreeNodeHtml_CB);
      }
      setTimeout('GetFreeNodeHtml()', 60000);
      </script>\n";
	  
echo "<div id=showexpusagefreenodes>\n";
echo   ShowFreeNodes($this_user, $experiment->Group());
echo "</div>\n";

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
            if (li) {
                li.style.backgroundColor = '#DDE';
                li.style.borderBottom = '1px solid #778';
            }

            li_current = 'li_' + which;
	    li = getObjbyName(li_current);
            if (li) {
                li.style.backgroundColor = 'white';
                li.style.borderBottom = '1px solid white';
                x_Show(which, 0, 0, Show_cb);
            }
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
        function ShowEvents() {
            x_Show('details', 1, 0, Show_cb);
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
        function ModifyAnno() {
            textarea = getObjbyName('annotation');
            x_ModifyAnno(textarea.value, ModifyAnno_cb);
        }
        function ModifyAnno_cb(val) {
        }
        function Setup() {
	    var urllocation = location.href; //find url parameter
	    if (urllocation && urllocation.indexOf('#') >= 0) {
                var which = urllocation.substr(urllocation.indexOf('#') + 1);

	        li = getObjbyName('li_' + which);
                if (!li) {
                    which = 'settings';
                }
                Show(which);
            }
            else {
	        li = getObjbyName(li_current);
                if (li) {
                    li.style.backgroundColor = 'white';
                    li.style.borderBottom = '1px solid white';
                }
            }
        }
      </script>\n";

#
# This is the topbar
#
echo "<div width=\"100%\" align=center>\n";
echo "<ul id=\"topnavbar\">\n";
echo "<li>
          <a href=\"#settings\" ".
               "class=topnavbar onfocus=\"this.hideFocus=true;\" ".
               "id=\"li_settings\" onclick=\"Show('settings');\">".
               "Settings</a></li>\n";
echo "<li>
          <a href=\"#vis\" ".
               "class=topnavbar onfocus=\"this.hideFocus=true;\" ".
               "id=\"li_vis\" onclick=\"Show('vis');\">".
               "Visualization</a></li>\n";
echo "<li>
          <a href=\"#nsfile\" ".
              "class=topnavbar onfocus=\"this.hideFocus=true;\"  ".
              "id=\"li_nsfile\" onclick=\"Show('nsfile');\">".
              "NS File</a></li>\n";
echo "<li>
          <a href=\"#details\" ".
              "class=topnavbar onfocus=\"this.hideFocus=true;\" ".
              "id=\"li_details\" onclick=\"Show('details');\">".
              "Details</a></li>\n";

if ($instance) {
    echo "<li>
              <a href=\"#anno\" ".
	          "class=topnavbar onfocus=\"this.hideFocus=true;\" ".
	          "id=\"li_anno\" onclick=\"Show('anno');\">".
                  "Annotation</a></li>\n";
}
if ($HAVE_USER_VIS) {
    echo "<li>
              <a href=\"#uservis\" ".
	          "class=topnavbar onfocus=\"this.hideFocus=true;\" ".
	          "id=\"li_uservis\" onclick=\"Show('uservis');\">".
                  "User Visualization</a></li>\n";
}
echo "</ul>\n";
echo "</div>\n";
echo "<div align=center id=topnavbarbottom>&nbsp</div>\n";

#
# Start out with details ...
#
echo "<div align=center width=\"100%\" id=\"showexp_visarea\">\n";
$experiment->Show();
echo "</div>\n";

if ($experiment->Firewalled() &&
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
	$panic_url = CreateURL("panicbutton", $experiment);
	
	echo "<br><a href='$panic_url'>
                 <img border=1 alt='panic button' src='panicbutton.gif'></a>";
	echo "<br><font color=red size=+2>".
	     " Press the Panic Button to contain your experiment".
	     "</font>\n";
    }
    echo "</center>\n";
}
SUBPAGEEND();

if ($instance) {
    $instance->ShowCurrentBindings();
}

#
# Dump the node information.
#
echo "<center>\n";
SHOWNODES($exp_pid, $exp_eid, $sortby, $showclass);
echo "</center>\n";

if ($isadmin) {
    echo "<center>
          <h3>Experiment Stats</h3>
         </center>\n";

    $experiment->ShowStats();
}

#
# Get the active tab to look right.
#
echo "<script type='text/javascript' language='javascript'>
      Setup();
      </script>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
