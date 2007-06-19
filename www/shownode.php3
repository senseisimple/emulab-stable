<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("node_defs.php");

#
# Standard Testbed Header
#
PAGEHEADER("Node Information");

#
# Only known and logged in users can do this.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$reqargs = RequiredPageArguments("node", PAGEARG_NODE);

# Need these below
$node_id = $node->node_id();

echo "<font size=+2>".
     "Node <b>$node_id</b></font>";

#
# Admin users can look at any node, but normal users can only control
# nodes in their own experiments.
#
if (! $isadmin &&
    ! $node->AccessCheck($this_user, $TB_NODEACCESS_MODIFYINFO)) {

    $power_id = "";
    $query_result = DBQueryFatal("select power_id from outlets ".
				 "where node_id='$node_id'");
    if (mysql_num_rows($query_result) > 0) {
	$row = mysql_fetch_array($query_result);
	$power_id = $row["power_id"];
    }
    if (STUDLY() && ($power_id == "mail")) {
	    SUBPAGESTART();
	    
	    SUBMENUSTART("Node Options");
	    WRITESUBMENUBUTTON("Update Power State",
			       "powertime.php3?node_id=$node_id");
	    SUBMENUEND();
	    $node->Show(SHOWNODE_NOPERM);
	    SUBPAGEEND();
    }
    else {
	    $node->Show(SHOWNODE_NOPERM);
    }
    PAGEFOOTER();
    return;
}

# If reserved, more menu options.
if (($experiment = $node->Reservation())) {
    $pid   = $experiment->pid();
    $eid   = $experiment->eid();
    $vname = $node->VirtName();
}
if ($experiment && $vname != "") {
    echo " (<b>".
	 "$vname.".
	 "<a href='showexp.php3?pid=$pid&eid=$eid'>$eid</a>.".
	 "<a href='showproject.php3?pid=$pid'>$pid</a>.".
	 "$OURDOMAIN".
	 "</b>)";
}	

echo "</font><br><br>\n";

SUBPAGESTART();
SUBMENUSTART("Node Options");

#
# Tip to node option
#
if ($node->HasSerialConsole()) {
    WRITESUBMENUBUTTON("Connect to Serial Line</a> " . 
	"<a href=\"faq.php3#tiptunnel\">(howto)",
	"nodetipacl.php3?node_id=$node_id");

    WRITESUBMENUBUTTON("Show Console Log",
		       "showconlog.php3?node_id=$node_id&linecount=500");
}

#
# SSH to option.
# 
if ($experiment) {
    WRITESUBMENUBUTTON("SSH to node</a> ".
		       "<a href='docwrapper.php3?docname=ssh-mime.html'>".
		       "(howto)", "nodessh.php3?node_id=$node_id");
}

#
# Edit option
#
WRITESUBMENUBUTTON("Edit Node Info",
		   "nodecontrol_form.php3?node_id=$node_id");

if ($isadmin ||
    $node->AccessCheck($this_user, $TB_NODEACCESS_REBOOT)) {
    if ($experiment) {
	WRITESUBMENUBUTTON("Update Node",
			   "updateaccounts.php3?pid=$pid&eid=$eid".
			   "&nodeid=$node_id");
    }
    WRITESUBMENUBUTTON("Reboot Node",
		       "boot.php3?node_id=$node_id");

    WRITESUBMENUBUTTON("Show Boot Log",
		       "bootlog.php3?node_id=$node_id");
}

if ($node->AccessCheck($this_user, $TB_NODEACCESS_LOADIMAGE)) {
    WRITESUBMENUBUTTON("Create a Disk Image",
		       "newimageid_ez.php3?formfields[node_id]=$node_id");
}

if (($isadmin ||
     $node->AccessCheck($this_user, $TB_NODEACCESS_READINFO)) &&
    ($node->TypeClass() == "robot")) {
    WRITESUBMENUBUTTON("Show Telemetry",
		       "telemetry.php3?node_id=$node_id",
		       "telemetry");
}

if ($isadmin || OPSGUY()) {
    WRITESUBMENUBUTTON("Show Node Log",
		       "shownodelog.php3?node_id=$node_id");
    WRITESUBMENUBUTTON("Show Node History",
		       "shownodehistory.php3?node_id=$node_id");
}
if ($experiment && ($isadmin || (OPSGUY()) && $pid == $TBOPSPID)) {
    WRITESUBMENUBUTTON("Free Node",
		       "freenode.php3?node_id=$node_id");
}

if ($isadmin || STUDLY() || OPSGUY()) {
    WRITESUBMENUBUTTON("Set Node Location",
		       "setnodeloc.php3?node_id=$node_id");
    WRITESUBMENUBUTTON("Update Power State",
		       "powertime.php3?node_id=$node_id");
}

if ($isadmin || STUDLY() || OPSGUY()) {
    WRITESUBMENUBUTTON("Modify Node Attributes",
                       "modnodeattributes_form.php3?node_id=$node_id");
}

if ($isadmin) {
    if (!$node->reserved_pid()) {
	WRITESUBMENUBUTTON("Pre-Reserve Node",
			   "prereserve_node.php3?node_id=$node_id");
    }
    else {
	WRITESUBMENUBUTTON("Clear Pre-Reserve",
			   "prereserve_node.php3?node_id=$node_id&clear=1");
    }
}

SUBMENUEND();

#
# Dump record.
# 
$node->Show(SHOWNODE_NOFLAGS);

SUBPAGEEND();

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>




