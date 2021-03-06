<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2003, 2004, 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

# a few constants
$DEF_LIMIT = 20;
$DEF_NUMPAGENO = 10;

$FLEXLAB_XMLRPC_SRV = 'ops.emulab.net';
$FLEXLAB_XMLRPC_SRV_PORT = 3993;

#
# Collect errors.
#
$opterrs = array();

#
# Only known and logged in users can get plab data.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

if (!$PLABSUPPORT) {
    PAGEHEADER("PlanetLab Metrics");
    echo "<p>This site does not have PlanetLab support configured.</p>";
    echo "<p>Email $TBMAILADDR_OPS if this confuses you!</p>";
    PAGEFOOTER();
    exit(0);
}

#
# Hacks -- unset $pid and $eid vars if they are '' before we check args!
# (needed so we can let users drop a previous pid/eid filter).
#
if (isset($_REQUEST['pid']) && $_REQUEST['pid'] == '') {
    unset($_REQUEST['pid']);
}
if (isset($_REQUEST['eid']) && $_REQUEST['eid'] == '') {
    unset($_REQUEST['eid']);
}

# Do similar for other arguments.
if (isset($_REQUEST['upnodefilter']) && $_REQUEST['upnodefilter'] == '') {
    unset($_REQUEST['upnodefilter']);
}
if (isset($_REQUEST['userquery']) && $_REQUEST['userquery'] == '') {
    unset($_REQUEST['userquery']);
}

# Make sure to add any new args here to pm_buildurl() as well.
$optargs = OptionalPageArguments( "experiment", PAGEARG_EXPERIMENT,
				  "cols",   PAGEARG_ALPHALIST,
				  "offset", PAGEARG_INTEGER,
				  "limit",  PAGEARG_INTEGER,
				  "upnodefilter", PAGEARG_STRING,
				  "pagelayout", PAGEARG_BOOLEAN,
				  "sortcols", PAGEARG_ALPHALIST,
				  "sortdir", PAGEARG_STRING,

                                  # Never fear.  We check this ourself since
                                  # it could contain either an alphanumeric
                                  # string, or a perl5 regex. NOTE:
                                  # only alphanum strings go in db queries;
                                  # regexes are handled within this page.
				  "hostfilter", PAGEARG_ANYTHING,

				  "selectable", PAGEARG_BOOLEAN,
				  "selectionlist", PAGEARG_ALPHALIST,
				  "newpgsel", PAGEARG_ARRAY,
				  "pgsel", PAGEARG_ARRAY,

                                  # We check this *extremely* copiously.
                                  # It actually gets tokenized and parsed and
                                  # we verify every last token against known 
                                  # or numeric values.
				  "userquery", PAGEARG_ANYTHING,

				  "flexlabfilter", PAGEARG_BOOLEAN,
				  "flexlabfcsize", PAGEARG_INTEGER,
				  "flexlabsave", PAGEARG_BOOLEAN,
				  "flexlabrecompute", PAGEARG_BOOLEAN,
				  "flexlabfcnodes", PAGEARG_ALPHALIST );

                                  # Unimplemented.
                                  # Verified, but never leaves php
                                  #"colrank", PAGEARG_ANYTHING);

# Yet to be implemented:
# * basic options
#     add and pcvm links; other?
# * advanced options
#     build-your-own rank (based off column weights, normalization to max or 
#       user-chosen ceil; each col should be able to be minimized/maximized in 
#       its contribution to the rank)

#
# Argument checks.
#

# first of all, only post-filter the data if absolutely necessary.
$mustpostfilter = 0;

# access control -- everybody can see stats for all nodes, but only members
# of a pid can see which nodes an eid in that pid has.
if (isset($experiment)) {
    if (!$experiment->AccessCheck($this_user, $TB_EXPT_READINFO)) {
	USERERROR("You do not have permission to view this experiment!",1);
    }

    # need these later
    $eid = $experiment->eid();
    $pid = $experiment->pid();
}

# check hostfilter -- if it is an alphanum string, we can send to db; else,
# it is a perl5 regexp and we have to filter in php.  Note that we also
# allow common hostname chars -,.,_ since the db can accept these...
if (!isset($hostfilter)) {
    $hf_regexp = 0;
}
elseif ($hostfilter == '' || preg_match("/^(\s+)$/",$hostfilter) > 0) {
    $hf_regexp = 0;
    unset($hostfilter);
}
elseif (preg_match("/^([0-9a-zA-Z\-_\.]+)$/",$hostfilter) == 1) {
    $hf_regexp = 0;
}
else {
    $hf_regexp = 1;
    # setup post-filtering so that we can match using regexps.
    $mustpostfilter = 1;
    #echo "needed regexp and postfilter<br>\n";
}

#
# Flexlab filter options:
#
if (isset($flexlabfilter) && $flexlabfilter) {
    if (isset($flexlabsave) && $flexlabsave 
	&& isset($flexlabrecompute) && $flexlabrecompute) {
        # unset recompute; save is the winner...
	unset($flexlabrecompute);
    }

    $mustpostfilter = 1;
}
else {
    if (isset($flexlabfcnodes)) {
	unset($flexlabfcnodes);
    }
}

#
# If filtering on pid/eid AND upnodefilter == emulab, we must postfilter the
# data (initial query will get the list of pid/eid nodes, then we filter out
# upnodes).  This is because a plab node isup according to Emulab if its 
# node status is up and if it is in the emulab-ops/plabnodes experiment.
#
if (isset($experiment) 
    && (isset($upnodefilter) && $upnodefilter == 'emulab')) {
    $mustpostfilter = 1;
}

#
# Standard Testbed Header
#
# Cheat and use the page args to influence the view.
$view = array();
if (isset($_REQUEST['pagelayout']) && $_REQUEST['pagelayout'] == "minimal") {
    $view = array ('hide_banner' => 1, 'hide_sidebar' => 1 );
}

PAGEHEADER("PlanetLab Metrics",$view);

#
# Some javascript to help us out.
#
?>
<script type='text/javascript' src='<?php echo $TBBASE; ?>/dynselect.js'></script>
<script type="text/javascript" language="javascript">

var divstate = new Array();

function isdef(obj) {
    if (typeof(obj) != "undefined") {
	return true;
    }
    return false;
}

function divinit(id) {
    var obj = document.getElementById(id);
    if (!isdef(obj)) {
	return false;
    }
    divstate[id] = new Object();
    divstate[id].origheight = obj.offsetHeight;
    divstate[id].vis = obj.style.visibility;
}

function divflipvis(id,linktxtid) {
    var obj = document.getElementById(id);
    if (!isdef(obj)) {
	return false;
    }
    if (!isdef(divstate[id])) {
	return false;
    }
    if (divstate[id].vis == "visible") {
	obj.style.height = "0px";
	obj.style.position = "absolute";
	divstate[id].vis = obj.style.visibility = "hidden";
    }
    else {
	obj.style.height = divstate[id].origheight+"px";
	//obj.style.height = obj.offsetHeight+"px";
	obj.style.position = "static";
	divstate[id].vis = obj.style.visibility = "visible";
    }

    linktxt = "Show";
    if (divstate[id].vis == "visible") {
	linktxt = "Hide";
    }
    if (linktxtid != null) {
	var lobj = document.getElementById(linktxtid);
	if (lobj != null) {
	    lobj.innerHTML = linktxt;
	}
    }
}

function init() {
    divinit('searchdiv');
    //    divinit('selectiondiv');

    //divflipvis('searchdiv');
    //divflipvis('legenddiv');
    //divflipvis('selectiondiv');
}

addLoadFunction(init);

function setElementValue(id,value) {
    if (id == null) {
	return false;
    }
    var obj = document.getElementById(id);
    if (!isdef(obj)) {
	return false;
    }

    obj.value = value;

    return true;
}

function setFormElementValue(form,id,value) {
    if (form == null || id == null) {
	return false;
    }
    var fobj = document.forms[form].elements[id];
    if (!isdef(fobj) ) {
    	return false;
    }
    
    fobj.value = value;

}

function setSelectionAllCheckboxes(form,selstate) {
    if (form == null) {
	return false;
    }

    var fobj = document.forms[form];
    for (var i = 0; i < fobj.elements.length; ++i) {
	if (fobj.elements[i].type == 'checkbox') {
	    fobj.elements[i].checked = selstate;
	}
    }

    return true;
}

function getFormElementValue(form,id) {
    if (form == null || id == null) {
	return '';
    }
    var fobj = document.forms[form].elements[id];
    if (!isdef(fobj)) {
	return '';
    }

    return fobj.value;
}
    

</script>
<?php

#
# Setup the viewable exp/pid mappings:
#

?>

<script type='text/javascript' language='javascript'>
/*
 * Some pid/eid array mappings:
 */
var exps = new Array();
// handle the case where user doesn't want a pid/eid
// also see later where we generate the javascript event that handles the
// pid selectbox change.
exps['No Project'] = new Array('No Experiment');

var expValues = new Array();
expValues['No Experiment'] = '';

<?php

# This is a sick query, but there does not appear to be anything in the 
# user, project, or experiment objects that will do the job.
# We want to find all pid/eids that have mapped plab nodes that the user 
# can read.
if ($isadmin) {
    $pequery = "
select e.pid,e.eid from experiments as e 
left join reserved as r on e.pid=r.pid and e.eid=r.eid 
left join nodes as n on r.node_id=n.node_id 
left join nodes as n2 on n.phys_nodeid=n2.node_id 
left join node_types as nt on n2.type=nt.type 
where nt.isplabphysnode=1 group by e.pid,e.eid";
}
else {
    $pequery = "
select e.pid,e.eid from group_membership as g 
left join experiments as e on g.pid=e.pid and g.pid=g.gid 
left join reserved as r on e.pid=r.pid and e.eid=r.eid 
left join nodes as n on r.node_id=n.node_id 
left join nodes as n2 on n.phys_nodeid=n2.node_id 
left join node_types as nt on n2.type=nt.type 
where g.uid='johnsond' and e.pid is not null and e.eid is not null 
  and nt.isplabphysnode=1 group by e.pid,e.eid";
}
$qres = DBQueryFatal($pequery);

$pidmap = array();

if (mysql_num_rows($qres) > 0) {
    while ($row = mysql_fetch_array($qres)) {
	$tpid = $row['pid'];
	$teid = $row['eid'];

	if (!array_key_exists($tpid,$pidmap)) {
	    $pidmap[$tpid] = array($teid);
	}
	else {
	    array_push($pidmap[$tpid],$teid);
	}
    }

    # now dump to an acceptable format in javascript, sigh
    foreach ($pidmap as $tpid => $ta) {
	echo "exps['$tpid'] = new Array(";
	$elmstr = '';
	foreach ($ta as $teid) {
	    $elmstr .= "'$teid',";
	}
	$elmstr = rtrim($elmstr,",");
	echo "$elmstr);\n";
    }
}

# Close off the script we started above.
?>

</script>

<?php

#
# Setup data source information (i.e., fields in our database that 
# can be displayed).
#

# A map of colnames the user can refer to, to their name in queries we issue.
# This mapping handles rvalues with table prefixes or ' as ' aliases.
$colmap = array( 'node_id' => 'pm.node_id',
		 'hostname' => 'pm.hostname',
		 'plab_id' => 'pm.plab_id',
                 'plc_name' => 'ppi.plc_name',
		 'site' => 'psm.site_name',
		 'plab_nodegroup' => 'png.nodegrouplist',
                 # emulab columns
		 'nodestatus' => 'ns.status',
		 'nodestatustime' => 'ns.status_timestamp',
		 'nettype' => 'nat.type',
		 'unavail' => 'pnhs.unavail',
		 'jitdeduction' => 'pnhs.jitdeduct',
		 'successes' => 'pnhs.succnum',
		 'failures' => 'pnhs.failnum',
                 # comon columns
		 'resptime' => 'pcd.resptime','uptime' => 'pcd.uptime',
		 'lastcotop' => 'pcd.lastcotop',
		 'date' => 'from_unixtime(pcd.date) as codate',
		 'drift' => 'pcd.drift','cpuspeed' => 'pcd.cpuspeed',
		 'busycpu' => 'pcd.busycpu','syscpu' => 'pcd.syscpu',
		 'freecpu' => 'pcd.freecpu','1minload' => 'pcd.1minload',
		 '5minload' => 'pcd.5minload','numslices' => 'pcd.numslices',
		 'liveslices' => 'pcd.liveslices','connmax' => 'pcd.connmax',
		 'connavg' => 'pcd.connavg','timermax' => 'pcd.timermax',
		 'timeravg' => 'pcd.timeravg','memsize' => 'pcd.memsize',
		 'memact' => 'pcd.memact','freemem' => 'pcd.freemem',
		 'swapin' => 'pcd.swapin','swapout' => 'pcd.swapout',
		 'diskin' => 'pcd.diskin','diskout' => 'pcd.diskout',
		 'gbfree' => 'pcd.gbfree','swapused' => 'pcd.swapused',
		 'bwlimit' => 'pcd.bwlimit','txrate' => 'pcd.txrate',
		 'rxrate' => 'pcd.rxrate' );

# An array to help color data sources so users can more easily figure out
# what is what.
$colsrc = array( 'resptime' => 'CoMon','uptime' => 'CoMon',
                 'lastcotop' => 'CoMon','date' => 'CoMon','drift' => 'CoMon',
		 'cpuspeed' => 'CoMon','busycpu' => 'CoMon',
		 'syscpu' => 'CoMon','freecpu' => 'CoMon','1minload' =>'CoMon',
		 '5minload' => 'CoMon','numslices' => 'CoMon',
                 'liveslices' => 'CoMon','connmax' => 'CoMon',
                 'connavg' => 'CoMon','timermax' => 'CoMon',
		 'timeravg' => 'CoMon','memsize' => 'CoMon',
                 'memact' => 'CoMon','freemem' => 'CoMon',
                 'swapin' => 'CoMon','swapout' => 'CoMon',
                 'diskin' => 'CoMon','diskout' => 'CoMon',
                 'gbfree' => 'CoMon','swapused' => 'CoMon',
                 'bwlimit' => 'CoMon','txrate' => 'CoMon','rxrate' => 'CoMon',
		 
		 'node_id' => 'Emulab','hostname' => 'Emulab',
                 'plc_name' => 'Emulab',
		 'plab_id' => 'Emulab','unavail' => 'Emulab',
		 'site' => 'Emulab','nettype' => 'Emulab',
		 'jitdeduction' => 'Emulab','successes' => 'Emulab',
                 'failures' => 'Emulab','nodestatus' => 'Emulab',
                 'nodestatustime' => 'Emulab','plab_nodegroup' => 'Emulab' );

# The legend for fields.  The comon data is quoted from
# http://summer.cs.princeton.edu/status/legend.html .
$coldoc = array( 'node_id' => 'Emulab physical node id.',
		 'hostname' => 'Node hostname.',
		 'plab_id' => 'PLC node id number.',
		 'plc_name' => 'PLC name.',
		 'site' => 'Site with which the node is affiliated.',
		 'nodestatus' => 'Emulab node status; up or down.',
		 'nodestatustime' => 'Most recent heartbeat time from the ' . 
		   'node to Emulab.',
		 'nettype' => 'Type of network node lives on.',
		 'unavail' => 'Unavailability percentage as calculated by ' . 
		   'Emulab.  Is increased if the node is often up and down.',
		 'jitdeduction' => 'Amount added to total unavailability ' . 
		   'percent; depends on if the node is often up and down.',
		 'successes' => 'Number of recent Emulab sliver creation ' . 
		   'successes on the node.',
		 'failures' => 'Number of recent Emulab sliver creation ' . 
		   'failures on the node.',
		 'resptime' => '"How long it took for the node to respond ' . 
		   'with the CoMon data."',
		 'uptime' => '"How long the node has been up since last reboot."',
		 'lastcotop' => '"How long it\'s been since we were able ' . 
		   'to query the CoTop sensor on the node."',
		 'date' => '"Reflects what time the node thinks it is, ' . 
		   'slightly adjusted for skew."',
		 'drift' => '"How far off from the median node time is ' . 
		   'this node\'s time."',
		 'cpuspeed' => '"The speed of the processor in gigahertz."',
		 'busycpu' => '"Gives the % of time the CPU is utilized."',
		 'syscpu' => '"Percentage of time the CPU is spending in ' . 
		   'the OS"',
		 'freecpu' => '"Indicates how much of the CPU a spin-loop ' . 
		   'was able to obtain"',
		 '1minload,5minload' => '"Load averages (# of runnable ' . 
		   'processes) for the past 1 and and 5 minutes. The ' . 
		   '1-minute value is actually the maximum 1-minute value ' . 
		   'over the past 5 minutes."',
		 'numslices' => '"Indicates the maximum number of slices ' . 
		   'reported by slicestat over the past 5 minutes, measured ' .
		   'once every 30 seconds."',
		 'liveslices' => '"The count of how many slices used at ' . 
		   'least one-tenth of one percent of the CPU in the last 5 ' .
		   'minutes, measured at 30-second intervals."',
		 'connmax,connavg' => '"We [CoMon] have a small client and ' . 
		   'server demon that performs a TCP accept every second. ' . 
		   'The Conn Max and Conn Avg fields show the maximum and ' . 
		   'average times of the connect, over the past 5 minutes. ' . 
		   'These values are in milliseconds."',
		 'timermax,timeravg' => '"On each node, we [CoMon] run a ' . 
		   'timer that tries to sleep for about 1ms at a time. The ' . 
		   'Timer Max field shows the maximum interval between ' . 
		   'timers and the Timer Avg field gives the average ' . 
		   'interval, over the past 5 minutes. The hope is that ' . 
		   'this gives some insight into the scheduler behavior. ' . 
		   'These values are in milliseconds."',
		 'memsize' => '"The node\'s memory size in GB."',
		 'memact' => '"Percentage of the memory is in the operating ' .
		   'system\'s "active" pool."',
		 'freemem' => '"The output of a test program that tries to ' . 
		   'grab 100MB of memory on the node."',
		 'swapin,swapout' => '"The amount of memory swapped in/out ' . 
		   'of disk per second, as obtained from vmstat."',
		 'diskin,diskout' => '"The blocks in/out per second, as ' . 
		   'obtained from vmstat."',
		 'gbfree' => '"The amount of free space left on the disk, ' . 
		   'in GB."',
		 'swapused' => '"The percentage of the swap space consumed."',
		 'bwlimit' => 'Node bandwidth limit.',
		 'txrate,rxrate' => '"Transmit and receive rates (in Kbps), ' .
		   'taken as a sum of per-slice activity."' );

# An array containing colors for data sources.
$colcol = array( 'Emulab' => '#cde4f3', 'CoMon' => '#ffedd7' );


# Some help info.
$help = array();
$help['project/experiment'] = 
    "Show only nodes in one of your swapped-in experiments.";
$help['alive nodes'] = 
    "Show only nodes that are considered alive by Emulab (had" . 
    " heartbeat in last 20min) or by CoMon (CoTop in last 10 mins).";
$help['hostname'] = "Show only nodes matching this alphanumeric string" . 
    " or Perl5 regular expression.";
$help['custom query'] = "Show only nodes whose data fields match this" . 
    " expression.  Expressions are simple boolean queries such as" . 
    " (nodestatus == up and unavail < 0.25).  You can use the operators" . 
    " '<','>','<=','>=','==','!='.  Barewords are considered strings; if" . 
    " you must encode whitespace, surround the text with quotes.";
$help['show/order fields'] = "Move fields from the left-side box to the" . 
    " right-side box; fields in the right-side box are displayed.  In the" . 
    " right-side box, move fields up and down to change left-to-right " . 
    " display order.";
$help['sort fields'] = "Sort returned data in either ascending or descending" .
    " order, in the order of the fields.  Move the fields up and down to" . 
    " change the ordering.";
$help['nodes per page'] = "Show only N nodes per page.";
$help['fully connected nodes from Flexlab data'] = 
    "Show N fully-connected nodes based on latest Flexlab NxN tests." . 
    " Possible nodes are chosen from the pool of nodes selected by any" . 
    " filters you have enabled.";
$help['save fully connected set without recomputing'] = 
    "Save the computed fully-connected set for additional filtering" . 
    "(saves slight computation delay, depending on size of set).";
$help['recompute fully-connected set'] = 
    "Force a recomputation of the fully-connected set based on the current" . 
    " filter set.";


# We have to fix which node id we select based on if we are querying the 
# reserved table (we need phys node id, but have to join reserved to nodes
# to get it).
if (isset($experiment)) {
    $colmap['node_id'] = 'n.phys_nodeid';
}

# Strip off the table prefixes or use 'as' aliases; mysql driver does not 
# seem to include them as column names in mysql_fetch_array (needed for 
# converting user-visible col names to db select-able col names).
#
# Also form a second array that DOES have table prefixes, but only has aliases
# (need this for the where clause).
$colmap_mysqlnames = array();
$colmap_mysqlnames_where = array();

foreach ($colmap as $k => $v) {
    $had_as = false;
    if (strpos($v," as ") != false) {
	$sa = explode(' as ',$v);
	$nv = $sa[1];
	$had_as = true;
    }
    else {
	$sa = explode('.',$v);
	$nv = $sa[0];
	if (count($sa) == 2) {
	    $nv = $sa[1];
	}
	elseif (count($sa) > 2) {
	    $nv = implode('.',array_slice($sa,1));
	}
    }

    $colmap_mysqlnames[$k] = $nv;
    if ($had_as) {
	$colmap_mysqlnames_where[$k] = $nv;
    }
    else {
	$colmap_mysqlnames_where[$k] = $v;
    }
}

# Default columns displayed, in this order.
$defcols = array( 'node_id','hostname','plab_nodegroup','nodestatus',
                  'nodestatustime','unavail','5minload','freemem',
                  'txrate','rxrate','date', );

# Default sort and included columns
$defsortcols = array( 'unavail','5minload' );

# Assign a default set of columns if unset.
if (!isset($cols) || count($cols) == 0) {
    $cols = $defcols;
}

# If selecting nodes, make sure that node_id is included in our queries.
if (isset($selectable) && $selectable) {
    $foundit = false;
    foreach ($cols as $c) {
	if ($c == 'node_id') {
	    $foundit = true;
	}
    }
    if (!$foundit) {
	$ta = array('node_id');
	foreach ($cols as $c) {
	    array_push($ta,$c);
	}
	$cols = $ta;
    }
}

# Assign a default set of sort columns and sort order if unset.
if (!isset($sortcols)) {
    $sortcols = $defsortcols;
    $sortdir = 'asc';
}

# Ensure all cols (vis/order and sort) are valid.  Remove them if not.
$tmpcols = array();
foreach ($cols as $sc) {
    if (array_key_exists($sc,$colmap)) {
	array_push($tmpcols,$sc);
    }
    else {
	array_push($opterrs,"Invalid display column '$sc'.");
    }
}
$cols = $tmpcols;

$tmpcols = array();
foreach ($sortcols as $sc) {
    if (array_key_exists($sc,$colmap)) {
        array_push($tmpcols,$sc);
    }
    else {
        array_push($opterrs,"Invalid sort column '$sc'.");
    }
}
$sortcols = $tmpcols;

# Ensure sortdir is valid (asc/desc).
if (isset($sortdir) && $sortdir != 'asc' && $sortdir != 'desc') {
    array_push($opterrs,"Invalid sort dir '$sortdir'; changing to 'asc'!");
    $sortdir = 'asc';
}

#
# Figure out if the user selected any new plab nodes, and update the
# selectionlist if so.
#
if (isset($selectionlist) && $selectionlist[0] == 'ALLINSEARCH') {
    ;
}
elseif ((isset($newpgsel) && count($newpgsel) > 0) 
	|| (isset($pgsel) && count($pgsel) > 0)) {
    if (!isset($pgsel)) {
	$pgsel = array();
    }
    if (!isset($newpgsel)) {
	$newpgsel = array();
    }
    if (!isset($selectionlist) || $selectionlist == '') {
	$selectionlist = array();
    }

    # compare newpgsel and pgsel; see which are new, and add new ones
    # to selectionlist
    $ra = array();
    $aa = array();
    foreach ($newpgsel as $i) {
	if (!in_array($i,$pgsel)) {
	    array_push($aa,$i);
	}
    }
    foreach ($pgsel as $os) {
	if ($os != '' && !in_array($os,$newpgsel)) {
	    array_push($ra,$os);
	}
    }
    foreach ($aa as $elm) {
	if (!in_array($elm,$selectionlist)) {
	    array_push($selectionlist,$elm);
	}
    }
    $da = array_diff($selectionlist,$ra);
    $selectionlist = $da;
}

#
# Grab the base types for all plc nodetypes.  Used in pm_buildqueryinfo below.
#
$basenodetypes = array();
$qres = DBQueryFatal("select type from node_types where isplabdslice=1");
while ($row = mysql_fetch_array($qres)) {
    array_push($basenodetypes,$row['type']);
}

if (empty($basenodetypes)) {
    echo "<p>This site has not added any PlanetLab-style node types.</p>";
    echo "<p>Email $TBMAILADDR_OPS if this confuses you!</p>";
    PAGEFOOTER();
    exit(0);
}

#
# Next, build query:
#

# Grab query parts
$qbits = pm_buildqueryinfo();
$select_s = $qbits[0]; $src_s = $qbits[1]; $filter_s = $qbits[2]; 
$sort_s = $qbits[3]; $group_s = $qbits[4]; $pag_s = $qbits[5];

# query that counts num data rows
$qcount = "select count(" . $colmap['node_id'] . ") as num" . 
    " from $src_s $filter_s $group_s";

# query that gets data (note that if we need to do postfiltering, we
# manually paginate the data!)
if ($mustpostfilter) {
    $q = "select $select_s from $src_s $filter_s $group_s $sort_s";
}
else {
    $q = "select $select_s from $src_s $filter_s $group_s $sort_s $pag_s";
}

#echo "qcount = $qcount<br><br>\n";
#echo "q = $q<br><br>\n";

$qres = DBQueryFatal($qcount);
if (mysql_num_rows($qres) != 1) {
    TBERROR("Unexpected number of rows in count query!");
}
$row = mysql_fetch_array($qres);
$totalrows = $row['num'];

#
# Grab the query data!  Hate to store it in memory, but because of the page
# layout we must.
#
$qres = DBQueryFatal($q);
$data = array();
while ($row = mysql_fetch_array($qres)) {
    array_push($data,$row);
}

#
# Post-db filtering
#
if ($mustpostfilter) {
    $data = pm_filterdata($data);
    $totalrows = count($data);

    # Now we must manually paginate (i.e., we just do offset/limit ourself).
    $i = 0;
    $finaldata = array();
    foreach ($data as $row) {
	if ($i >= $offset && $i < ($offset+$limit)) {
	    array_push($finaldata,$row);
	}
	++$i;
    }

    $data = $finaldata;
}

#
# Select all if desired.
#
if (isset($selectionlist) && count($selectionlist) == 1 
    && $selectionlist[0] == 'ALLINSEARCH') {
    $selectionlist = array();    
    foreach ($data as $row) {
	array_push($selectionlist,$row[$colmap_mysqlnames['node_id']]);
    }
}

#
# Draw nondata part of page
#
pm_shownondata();

echo "<br><br>";

#
# Finally, execute main data query and display resulting data.
#
pm_showtable($totalrows,$data);

#
# Say what it all means.
#
pm_showlegend();

#
# Provide some help.
#
pm_showhelp();

#
# Standard Testbed Footer
#
PAGEFOOTER();


#
# Utility functions.
#

#
# Filters the rows returned by a sql query and returns a new set of
# rows that the filter allows though.
#
function pm_filterdata($data) {
    global $hostfilter,$hf_regexp;
    global $colmap_mysqlnames;
    global $upnodefilter,$experiment;
    global $flexlabfilter,$flexlabfcsize,$flexlabsave,$flexlabrecompute;
    global $flexlabfcnodes;
    global $FLEXLAB_XMLRPC_SRV,$FLEXLAB_XMLRPC_SRV_PORT;

    global $opterrs;

    # Array of node_ids; must be reduced after each filter.
    $remaining_nodes = array();

    #
    # Filter all rows based on regexp (expensive but no good way).
    #
    if ($hf_regexp) {
	$rnatmp = array();
	$rnodes = array();
	foreach ($data as $row) {
	    if (preg_match("/$hostfilter/",
			   $row[$colmap_mysqlnames['hostname']]) > 0) {
		array_push($rnatmp,$row);
		$rnodes[$row[$colmap_mysqlnames['node_id']]] = 1;
	    }
	}
	$remaining_nodes = $rnatmp;
    }
    else {
	$remaining_nodes = $data;
    }

    #
    # If filtering on $experiment AND $upnodefilter == 'emulab', we must 
    # postfilter to see if the nodes really were in emulab-ops/plabnodes.
    #
    if (isset($experiment) 
	&& (isset($upnodefilter) && $upnodefilter == 'emulab')) {
        #echo "FOO: rns = " . count($remaining_nodes) . "<br>\n";
	$upnodes = array();
	$pnq = "select n.node_id from nodes as n" . 
	    " left join reserved as r on n.node_id=r.node_id" . 
	    " where n.type='pcplabphys'" . 
	    "  and !(r.pid='emulab-ops' and r.eid='hwdown')";

	$pnqres = DBQueryFatal($pnq);
	while ($row = mysql_fetch_array($pnqres)) {
	    array_push($upnodes,$row['node_id']);
	}

	$rnatmp = array();
	foreach ($remaining_nodes as $row) {
	    if (in_array($row[$colmap_mysqlnames['node_id']],$upnodes)) {
		array_push($rnatmp,$row);
	    }
	}
	$remaining_nodes = $rnatmp;
        #echo "FOO: rns = " . count($remaining_nodes) . "<br>\n";
    }

    #
    # Flexlab "goodnodes" filter.  Contacts xmlrpc server on ops and gets a 
    # list of fully-connected nodes.
    #
    $rnatmp = array();
    if (isset($flexlabfilter) && $flexlabfilter) {
	if (isset($flexlabsave) && $flexlabsave 
	    && isset($flexlabfcnodes)) {
            # Do not make another expensive xmlrpc call -- just filter 
            # against the current flexlab set.
	    foreach ($remaining_nodes as $row) {
		if (in_array($row[$colmap_mysqlnames['node_id']],
			     $flexlabfcnodes)) {
		    array_push($rnatmp,$row);
		}
	    }
	}
	else {
            # Make the call.
	    $xargs = array( 'size' => $flexlabfcsize );
	    $ta = array();
	    foreach ($remaining_nodes as $row) {
		array_push($ta,$row[$colmap_mysqlnames['node_id']]);
	    }
	    if (count($ta) > 0) {
		$xargs['nodefilter'] = $ta;
		$xargs['filtertype'] = True;
	    }
	    $response = do_xmlrpc($FLEXLAB_XMLRPC_SRV,$FLEXLAB_XMLRPC_SRV_PORT,
				  'flexlab.getFullyConnectedSet',$xargs);
	    if (is_array($response) && is_array($response[1]) && $response[0] 
		&& !xmlrpc_is_fault($response[1])) {
                #echo "xresp = '" . implode(',',$response[1]) . "'<br>\n";
		$rnatmp = array();
		$flexlabfcnodes = array();
		foreach ($remaining_nodes as $row) {
		    if (in_array($row[$colmap_mysqlnames['node_id']],
				 $response[1])) {
			array_push($rnatmp,$row);
			array_push($flexlabfcnodes,
				   $row[$colmap_mysqlnames['node_id']]);
		    }
		}
	    }
	    else {
		if (is_array($response) || !is_array($response[1])) {
		    array_push($opterrs,"XMLRPC Server Error: " . 
			       $response[1] . "!\n");
		}
		elseif (!$response[0]) {
		    array_push($opterrs,
			       "Error in XMLRPC transport: " . $response[1]);
		}
		elseif (xmlrpc_is_fault($response[1])) {
		    array_push($opterrs,
			       "XMLRPC fault: code=".$response[1]['faultCode'].
			       "; msg=".$response[1]['faultString']);
		}
		else {
		    array_push($opterrs,"Completely unknown XMLRPC error!");
		}
		$rnatmp = $remaining_nodes;
		$flexlabfcnodes = array();
	    }

            # Now make sure that the save checkbox gets marked, and that 
            # recompute is off (try to save computation and keep result set
            # stable).
	    if (isset($flexlabrecompute)) {
		$flexlabrecompute = False;
	    }

            # Auto-save if any nodes were returned.
	    if (count($flexlabfcnodes) > 0) {
		$flexlabsave = True;
	    }
	}
	$remaining_nodes = $rnatmp;
    }
    
    return $remaining_nodes;
}

function do_xmlrpc($host,$port,$method,$argdict) {
    $xreq = xmlrpc_encode_request($method,$argdict);
    #echo "xreq = '<!-- " . $xreq . " -->'<br>\n";
    $xheaders = array( 'Content-type: text/xml',
		       "Content-length: " . strlen($xreq) );
    $c = curl_init();
    curl_setopt($c,CURLOPT_URL,"http://$host:$port/");
    curl_setopt($c,CURLOPT_RETURNTRANSFER,1);
    curl_setopt($c,CURLOPT_TIMEOUT,120);
    curl_setopt($c,CURLOPT_HTTPHEADER,$xheaders);
    curl_setopt($c,CURLOPT_POSTFIELDS,$xreq);

    $xresp = curl_exec($c);
    if (curl_errno($c)) {
	$errstr = curl_error($c);
	curl_close($c);
	return array(False,$errstr);
    }
    else {
	curl_close($c);
	return array(True,xmlrpc_decode($xresp));
    }
}

function pm_showhelp() {
    global $help;

    echo "<h3><a name='help'>Search Help</a></h3>\n";
    echo "<p>Filters:</p>\n";
    echo "<p><ul>\n";
    foreach ($help as $hi => $hv) {
	$link_hi = str_replace(" ","_",$hi);
	echo "<li><b><a name='$link_hi'>$hi</a></b> $hv</li>\n";
    }
    echo "</ul></p>\n";
}

function pm_showlegend() {
    global $colsrc,$coldoc;

    echo "<h3><a name='legend'>Data Legend</a></h3>\n";
    echo "<p>Emulab data:</p>\n";
    echo "<p><ul>\n";
    foreach ($coldoc as $col => $doc) {
	$rcol = $col;
	if (strstr($col,',')) {
	    $ta = explode(',',$col);
	    $rcol = $ta[0];
	}
	if ($colsrc[$rcol] == 'Emulab') {
	    echo "<li><b>$col</b> " . $coldoc[$col];
	}
    }
    echo "</p></ul>\n";
    echo "<p>CoMon data (quotes are from <a href='" . 
	"http://summer.cs.princeton.edu/status/legend.html'>the CoMon " . 
	"legend</a>):</p>\n";
    echo "<p><ul>\n";
    foreach ($coldoc as $col => $doc) {
	$rcol = $col;
	if (strstr($col,',')) {
	    $ta = explode(',',$col);
	    $rcol = $ta[0];
	}
	if ($colsrc[$rcol] == 'CoMon') {
	    echo "<li><b>$col</b> " . $coldoc[$col];
	}
    }
    echo "</p></ul>\n";
}

#
# Show the non-data part of the page:
#
function pm_shownondata() {
    global $pagelayout,$selectable;
    global $opterrs;

    echo "<table class='stealth'>\n";
    echo "<tr>\n";
    echo "<td class='stealth' valign='top' width='1%'>\n";
    SUBMENUSTART("Page Options");
    if (!isset($selectable) || !$selectable) {
	$url = pm_buildurl(array( 'selectable' => 'yes' ));
	WRITESUBMENUBUTTON("Select Nodes",$url);
    }
    else {
	$url = pm_buildurl(array( 'selectable' => 'no' ));
	WRITESUBMENUBUTTON("Hide Selection",$url);
    }
    WRITESUBMENUBUTTON("<span id='searchdivtw'>Hide</span> Search",
		       "javascript:divflipvis(\"searchdiv\",\"searchdivtw\")");
    if (isset($pagelayout) && $pagelayout == 'minimal') {
	$url = pm_buildurl(array('pagelayout' => ''));
	WRITESUBMENUBUTTON("Show Sidebar",$url);
    }
    else {
	$url = pm_buildurl(array('pagelayout' => 'minimal'));
	WRITESUBMENUBUTTON("Hide Sidebar",$url);
    }
    WRITESUBMENUDIVIDER();
    WRITESUBMENUBUTTON("Data Legend","#legend");
    WRITESUBMENUBUTTON("Search Help","#help");
    # Ugh -- cannot use SUBMENUEND cause it inserts a weird <td></td> combo
    # SUBMENUEND();
    #
    # XXX: commented out until I can figure this out...
    #
    #if (1) {
    #SideBarSubMenuRender();
    #}
    #else {
        echo "    </ul>\n";
        echo "    <!-- end submenu -->\n";
        echo "</td>\n";
        echo "<td class='stealth' halign='left' valign='top' width='99%'>\n";
    #}
    if (count($opterrs) > 0) {
	echo "<span style='color: red'>Errors:<br>\n";
	foreach ($opterrs as $e) {
	    echo "&nbsp;&nbsp;$e<br>\n";
	}
	echo "</span><br>\n";
    }
    pm_showsearch();
    if (isset($selectable) && $selectable) {
	echo "<br>\n";
	pm_showselection();
    }
    echo "</td>\n";
    echo "</tr>\n";
    echo "</table>\n";
}

#
# Show the selection box
#
function pm_showselection() {
    global $selectionlist;

    echo "<div style='padding: 8px; padding-left: 12px; padding-right: 12px;";
    echo " border: 2px solid silver; visibility: visible; margin-left: 4px'";
    echo " id='selectiondiv'>\n";

    echo "<form name='nodeeditsel' id='nodeeditsel' action='plabmetrics.php3' method='post'>\n";
    echo pm_gethiddenfields('nodeeditsel');

    echo "<b>Selected nodes</b>:<br>\n";

    if (!isset($selectionlist)) {
	echo "None.\n";
    }
    else {
	echo "<textarea name='selectionlist' rows='4' cols='64'>\n";
	echo implode(', ',$selectionlist);
	echo "</textarea>\n";
	echo "<br>\n";
	echo "<input type='submit' value='Save Selection Edits'>\n";
	echo "<input type='button' value='Clear Selection'" . 
	    " onClick='setFormElementValue(\"nodeeditsel\"," .
	    " \"selectionlist\",\"\")'>\n";
    }
    echo "</form>\n";

    # We need a separate mini-form that submits to plab_ez so that we can
    # open a new window without using javascript (only the <form> element
    # can accept a target attribute).
    if (isset($selectionlist)) {
	echo "<form name='toplabez' id='toplabez' action='plab_ez.php3'" . 
	    " method='post' target='_blank'" . 
	    " onSubmit='setFormElementValue(" . 
	    "   \"toplabez\",\"formfields[nodelist]\"," . 
	    "   (getFormElementValue(\"nodeeditsel\",\"selectionlist\")).replace(/\\,\s*/g,\" \"))'>\n";
        # plab_ez in advanced mode expects to see all these vars.
	echo "<input type='hidden' name='formfields[nodelist]' value=''>\n";
	echo "<input type='hidden' name='advanced' value='yes'>\n";
        # This is a hack.  We allow the submit button value to be submitted
        # to plab_ez so that plab_ez will figure out that it is supposed to
        # be in advanced mode.  Before changing the value (name) of this 
        # button, make sure you understand how plab_ez decides if it is in 
        # advanced mode.
	echo "<input type='submit' name='submit' value='Create PlanetLab Slice'>\n";
	echo "</form>\n";
    }    

    echo "</div>\n";
}

#
# Show the search box.
#
function pm_showsearch() {
    global $limit,$DEF_LIMIT,$offset;
    global $upnodefilter,$pid,$eid,$sortcols,$cols,$sortdir;
    global $pidmap,$colmap,$colcol,$colsrc;
    global $hostfilter;
    global $pagelayout;
    global $userquery;
    global $flexlabfilter,$flexlabfcsize,$flexlabsave,$flexlabrecompute;
    global $flexlabfcnodes;

    echo "<div style='padding: 8px; padding-left: 12px; padding-right: 12px;";
    echo " border: 2px solid silver; visibility: visible; margin-left: 4px'";
    echo " id='searchdiv'>\n";
    echo "<form name='plsearchform' id='plsearchform'" .
        " action='plabmetrics.php3' method='post' " .
        # We manually set a comma-delineated string comprised of the col names
        # in the display cols select box.  This way, we still only must handle
        # the cols var, instead of both the cols and selcols[] vars on 
        # search box post vs url get.
        " onSubmit='setFormElementValue(\"plsearchform\",\"cols\",".
	"dynselect.getOptsAsDelimString(document.getElementById(\"selcols\"),".
	"\",\",false)); " . 
	"setFormElementValue(\"plsearchform\",\"sortcols\",".
	"dynselect.getOptsAsDelimString(document.getElementById(\"selsortcols\"),".
	"\",\",false));" . "'" . ">\n";
    
    # Also includes hidden vals for col selection and sorting.
    echo pm_gethiddenfields('plsearchform');
    
    # Only draw pid/exp boxes if the user can access swapped-in experiments 
    # with pl nodes.
    if (count($pidmap)) {
	echo "Show <b>only nodes in project/experiment</b>";
	echo " <a href='#project/experiment'>[?]</a>";
	echo ":\n";
	echo "<select name='pid' id='pid'";
	echo " onChange='dynselect.setOptions(";
	echo "   document.getElementById(\"eid\"),";
	echo "   exps[dynselect.getSelectedOption(document.getElementById(\"pid\"))],";
	echo "   null,expValues)'>\n";
	echo "  <option value=''";
	$selpid = '';
	if (!isset($pid) || $pid == '') {
	    echo " selected";
	}
	echo ">No Project</option>\n";
	foreach (array_keys($pidmap) as $tpid) {
	    echo "  <option value='$tpid'";
	    if (isset($pid) && $pid == $tpid) {
		echo " selected";
		$selpid = $tpid;
	    }
	    echo ">$tpid</option>\n";
	}
	echo "</select>\n";
	echo "<select name='eid' id='eid'>\n";
	if ($selpid == '') {
	    echo "  <option value='' selected>No Experiment</option>\n";
	}
	else {
	    foreach ($pidmap[$selpid] as $teid) {
		echo "  <option value='$teid' ";
		if (isset($eid) && $eid == $teid) {
		    echo " selected";
		}
		echo ">$teid</option>\n";
	    }
	}
	echo "</select>\n";
	echo "<br>\n";
    }

    # handle basic upnodefiltering
    echo "Show <b>only nodes considered alive</b> ";
    echo " <a href='#alive_nodes'>[?]</a>";
    echo " by ";
    echo "<input type='checkbox' name='upnodefilter' value='emulab'";
    if (isset($upnodefilter) && $upnodefilter == 'emulab') {
	echo " checked";
    }
    echo " /> Emulab ";
    echo "<input type='checkbox' name='upnodefilter' value='comon'";
    if (isset($upnodefilter) && $upnodefilter == 'comon') {
	echo " checked";
    }
    echo " /> CoMon\n";
    echo "<br>\n";

    # hostname filtering (not included in normal user query because
    # we support perl5 regexps for filtering on the data as post-db)
    echo "Show only <b>hostnames that match</b> ";
    echo " <a href='#hostname'>[?]</a> ";
    $thf = '';
    if (isset($hostfilter)) {
	$thf = $hostfilter;
    }
    echo "<input type='text' value='$thf' name='hostfilter' size='25'>\n";
    echo "<br>\n";

    # custom user query
    echo "Filter with <b>custom query</b> ";
    echo " <a href='#custom_query'>[?]</a>";
    echo ": \n";
    $tuq = '';
    if (isset($userquery)) {
	$tuq = $userquery;
    }
    echo "<input type='text' value='$tuq' name='userquery' size='64'>\n";
    echo "<br>\n";


    # choose which columns are shown and what the column order should be
    # first do the headers -- col order and col sort get munged here!
    echo "<table class='stealth'>\n";
    echo "<tr>\n";
    echo "<td class='stealth' colspan='4'>\n";
    echo "<b>Show and reorder</b> ";
    echo " <a href='#show/order_fields'>[?]</a>";
    echo " fields:<br>\n";
    echo "</td>\n";
    # space
    echo "<td class='stealth' colspan='4'>\n";
    echo "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;";
    echo "</td>\n";
    echo "<td class='stealth' colspan='4'>\n";
    echo "<b>Sort selected</b> ";
    echo " <a href='#sort_fields'>[?]</a>";
    echo " fields:\n";
    echo "</td>\n";
    echo "</tr>\n";
    
    echo "<tr>\n"; 
    echo "<td class='stealth'>\n";
    echo "<select name='acols' id='acols' multiple size='6'>\n";
    # Compute the diff between avail cols and the current col selection.
    # avail cols go in the left box, current shown cols in the right.
    $da = array_diff(array_keys($colmap),$cols);
    foreach ($da as $foo) {
	$colstr = $colcol[$colsrc[$foo]];
	echo "  <option style='background-color: $colstr' value='$foo'>";
	echo "$foo</option>\n";
    }
    echo "</select>\n";
    echo "</td>\n";
    echo "<td class='stealth'>\n";
    echo "<input type='button' name='rb1' value='>>' " . 
	" onClick='dynselect.moveSelectedOptions(" . 
	"document.getElementById(\"acols\")," . 
	"document.getElementById(\"selcols\"))'" . ">\n";
    echo "<br>\n";
    echo "<input type='button' name='lb1' value='<<' " . 
	" onClick='dynselect.moveSelectedOptions(" . 
	"document.getElementById(\"selcols\")," . 
	"document.getElementById(\"acols\"))'" . ">\n";
    echo "<br>\n";
    echo "</td>\n";
    echo "<td class='stealth'>\n";
    echo "<select name='selcols' id='selcols' multiple size='6'>\n";
    foreach ($cols as $foo) {
	$colstr = $colcol[$colsrc[$foo]];
	echo "  <option style='background-color: $colstr' value='$foo'>";
	echo "$foo</option>\n";
    }
    echo "</select>\n";
    echo "</td>\n";
    echo "<td class='stealth'>\n";
    echo "<input type='button' name='up1' value='up' " . 
	" onClick='dynselect.moveSelectedOptionsVertically(" . 
	"document.getElementById(\"selcols\"),0)'" . ">\n";
    echo "<br>\n";
    echo "<input type='button' name='down1' value='down' " . 
	" onClick='dynselect.moveSelectedOptionsVertically(" . 
	"document.getElementById(\"selcols\"),1)'" . ">\n";

    # space
    echo "<td class='stealth' colspan='4'>\n";
    echo "&nbsp;&nbsp;&nbsp;&nbsp;";
    echo "</td>\n";

    # now widgets for column sort (note that this is different from order)
    echo "<td class='stealth'>\n";
    echo "<select name='acols2' id='acols2' multiple size='6'>\n";
    # Compute the diff between avail cols and the current col selection.
    # avail cols go in the left box, current shown cols in the right.
    $da2 = array_diff(array_keys($colmap),$sortcols);
    foreach ($da2 as $foo) {
	$colstr = $colcol[$colsrc[$foo]];
	echo "  <option style='background-color: $colstr' value='$foo'>";
	echo "$foo</option>\n";
    }
    echo "</select>\n";
    echo "</td>\n";
    echo "<td class='stealth'>\n";
    echo "<input type='button' name='rb2' value='>>' " . 
	" onClick='dynselect.moveSelectedOptions(" . 
	"document.getElementById(\"acols2\")," . 
	"document.getElementById(\"selsortcols\"))'" . ">\n";
    echo "<br>\n";
    echo "<input type='button' name='lb2' value='<<' " . 
	" onClick='dynselect.moveSelectedOptions(" . 
	"document.getElementById(\"selsortcols\")," . 
	"document.getElementById(\"acols2\"))'" . ">\n";
    echo "<br>\n";
    echo "</td>\n";
    echo "<td class='stealth'>\n";
    echo "<select name='selsortcols' id='selsortcols' multiple size='6'>\n";
    foreach ($sortcols as $foo) {
	$colstr = $colcol[$colsrc[$foo]];
	echo "  <option style='background-color: $colstr' value='$foo'>";
	echo "$foo</option>\n";
    }
    echo "</select>\n";
    echo "</td>\n";
    echo "<td class='stealth'>\n";
    echo "<input type='button' name='up2' value='up' " . 
	" onClick='dynselect.moveSelectedOptionsVertically(" . 
	"document.getElementById(\"selsortcols\"),0)'" . ">\n";
    echo "<br>\n";
    echo "<input type='button' name='down2' value='down' " . 
	" onClick='dynselect.moveSelectedOptionsVertically(" . 
	"document.getElementById(\"selsortcols\"),1)'" . ">\n";
    echo "<br><br>\n";
    echo "<select name='sortdir' id='sortdir'>\n";
    echo "  <option value='asc' ";
    if ($sortdir == 'asc') {
	echo " selected";
    }
    echo ">asc</option>\n";
    echo "  <option value='desc' ";
    if ($sortdir == 'desc') {
	echo " selected";
    }
    echo ">desc</option>\n";
    echo "</select>\n";
    echo "</td>\n";

    echo "</tr>\n";

    echo "</table>\n";

    # handle limit/offset
    $defLimits = array( '10' => 10,'20' => 20,'50' => 50,
		        '100' => 100,'All' => 0 );
    echo "Show ";
    echo "<select name='limit' id='limit'";
    # XXX
    # if the user changes the limit we change the offset to 0 so they
    # do not get funky page offset stuff
    echo "  onChange='setFormElementValue(\"plsearchform\",\"offset\",0)'>\n";
    foreach ($defLimits as $limk => $limv) {
	echo "  <option value='$limv' ";
	if ($limv == $limit) {
	    echo " selected";
	}
	echo ">$limk</option>\n";
    }
    echo "</select>\n";
    echo " PlanetLab <b>nodes per page</b> ";
    echo " <a href='#nodes_per_page'>[?]</a>";
    echo "\n";
    echo "<br>\n";

    # flexlab filter options
    $ffstate = "";
    if (isset($flexlabfilter) && $flexlabfilter) {
	$ffstate = ' checked';
    }
    echo "<input type='checkbox' name='flexlabfilter' value='yes'$ffstate>\n";
    echo " Filter against <b>Flexlab</b> data: ";
    echo "<br>\n";
    echo "&nbsp;&nbsp;&nbsp;&nbsp;";
    echo " Select ";
    $ffsize = 3;
    if (isset($flexlabfcsize)) {
	$ffsize = $flexlabfcsize;
    }
    echo "<input type='text' name='flexlabfcsize' size='4' value='$ffsize'>\n";
    echo " fully-connected nodes (one per site) ";
    echo " <a href='#fully_connected_nodes_from_Flexlab_data'>[?]</a>";
    echo "<br>";
    echo "&nbsp;&nbsp;&nbsp;&nbsp;";
    $ffstate = '';
    if (isset($flexlabsave) && $flexlabsave) {
	$ffstate = ' checked';
    }
    echo "<input type='checkbox' name='flexlabsave' value='yes'$ffstate>\n";
    echo " Save set <b>without recomputing</b> ";
    echo " <a href='#save_fully_connected_set_without_recomputing'>[?]</a>";
    echo " <br>\n";
    echo "&nbsp;&nbsp;&nbsp;&nbsp;";
    $ffstate = '';
    if (isset($flexlabrecompute) && $flexlabrecompute) {
	$ffstate = ' checked';
    }
    echo "<input type='checkbox' name='flexlabrecompute' value='yes'$ffstate>\n";
    echo " Force set <b>recomputation</b> ";
    echo " <a href='#recompute_fully-connected_set'>[?]</a>";

    echo "<br><br>\n";

    # form buttons
    echo "<input type='submit' value='Search'>\n";
    
    # finish the search box
    echo "</form>\n";
    echo "</div>\n";
}

#
# Any page arguments that you want preserved (i.e., 
# (Optional|Required)PageArguments ones) should be put in here.
# This is where we build urls that the various clickable links have.
# If you need to override any of the defaults, or add more, pass an array 
# with key/value params.
#
function pm_buildurl() {
    $pageargs = array( 'pid','eid','cols','sortcols','sortdir',
                       'upnodefilter','pagelayout','limit','offset',
		       'hostfilter','userquery',
		       'selectable','selectionlist',
		       'flexlabfilter','flexlabsave','flexlabfcsize',
		       'flexlabfcnodes' );

    $params = array();
    foreach ($pageargs as $pa) {
        if (isset($GLOBALS[$pa])) {
            $params[$pa] = $GLOBALS[$pa];
        }
    }

    if (func_num_args() > 0) {
        $aa = func_get_args();
        foreach ($aa[0] as $ok => $ov) {
            $params[$ok] = $ov;
        }
    }

    $retval = "plabmetrics.php3?";
    foreach ($params as $k => $v) {
        if (is_array($v)) {
            $v = '' . implode(',',$v);
        }
        $retval .= "$k=" . urlencode($v) . "&";
    }
    $retval = rtrim($retval,'&');

    return $retval;
}

#
# Parameterized by form name because the forms each have some of the values
# as non-hidden controls.
#
function pm_gethiddenfields($whichform,$override=array()) {
    $pageargs = array();
    if ($whichform == 'plsearchform') {
	$pageargs = array('pagelayout','offset',
			  'cols','sortcols',
			  'selectable','selectionlist',
			  'flexlabfcnodes');
    }
    elseif ($whichform == 'nodesel') {
	$pageargs = array('pid','eid','cols','sortcols','sortdir',
			  'upnodefilter','pagelayout','hostfilter',
			  'selectable','selectionlist','userquery',
			  'offset','limit',
			  'flexlabfilter','flexlabsave','flexlabfcsize',
			  'flexlabfcnodes');
    }
    elseif ($whichform == 'nodeeditsel') {
	$pageargs = array('pid','eid','cols','sortcols','sortdir',
			  'upnodefilter','pagelayout','hostfilter',
			  'selectable','userquery',
			  'offset','limit',
			  'flexlabfilter','flexlabsave','flexlabfcsize',
			  'flexlabfcnodes');
    }

    $params = array();
    foreach ($pageargs as $pa) {
        if (isset($GLOBALS[$pa])) {
            $params[$pa] = $GLOBALS[$pa];
        }
    }

    foreach ($override as $ok => $ov) {
	$params[$ok] = $ov;
    }

    $retval = "";
    foreach ($params as $k => $v) {
        if (is_array($v)) {
            $v = implode(',',$v);
        }
        # Include the id field just in case javascripts touch these.
        $retval .= "<input type='hidden' name='$k' id='$k' value='$v'>\n";
    }

    return $retval;
}    

function pm_getpaginationlinks($totalrows) {
    global $offset,$limit,$DEF_NUMPAGENO;

    $retval = '';

    if ($limit > 0) {
	$pagecount = ceil($totalrows / $limit);
	$pageno = floor($offset / $limit) + 1;
    }
    else {
	$pagecount = 1;
	$pageno = 1;
    }

    $retval .= "Page $pageno of $pagecount &mdash; ";
    if ($pageno > 1) {
        $url = pm_buildurl(array('offset' => 0));
        $retval .= "<a href='$url'>&lt;&lt;</a> ";
        $url = pm_buildurl(array('offset' => ($offset - $limit)));
        $retval .= "<a href='$url'>&lt;</a> ";
    }
    else {
        $retval .= "<< < ";
    }
    # print DEF_NUMPAGENO max, centered around the current page if possible
    if ($DEF_NUMPAGENO > $pagecount) {
        $startpn = 1;
        $stoppn = $pagecount;
    }
    else {
        $startpn = $pageno - floor(($DEF_NUMPAGENO - 1) / 2);
        $stoppn = $startpn + ($DEF_NUMPAGENO - 1);
        if ($startpn < 1) {
            $startpn = 1;
            $stoppn = $startpn + ($DEF_NUMPAGENO - 1);
            $stoppn = ($stoppn > $pagecount)?$pagecount:$stoppn;
        }
        elseif ($stoppn > $pagecount) {
            $stoppn = $pagecount;
            $startpn = $stoppn - ($DEF_NUMPAGENO - 1);
        }
    }
    for ($i = $startpn; $i <= $stoppn; ++$i) {
        $url = pm_buildurl(array('offset' => ($i - 1) * $limit));
        $retval .= "<a href='$url'>$i</a> ";
    }

    if ($pageno < $pagecount) {
        $url = pm_buildurl(array('offset' => ($offset + $limit)));
        $retval .= "<a href='$url'>&gt;</a> ";
        $url = pm_buildurl(array('offset' => ($pagecount-1)*$limit));
        $retval .= "<a href='$url'>&gt;&gt;</a> ";
    }
    else {
        $retval .= "> >> ";
    }

    return $retval;
}

#
# Assemble query chunks.  Returns an array of query subparts.  Each element is
# intended to go into the query like this:
#   select [0] from [1] [2] [3] [4] 
# where [1] is a table source with joins; [2] is a where filter; 
# [3] is a sort; and [4] is a limit/offset thing for pagination.
#
function pm_buildqueryinfo() {
    global $cols,$colmap,$experiment,$upnodefilter,$sortcols,$sortdir;
    global $colmap_mysqlnames;
    global $limit,$offset;
    global $pid,$eid;
    global $DEF_LIMIT;
    global $hostfilter,$hf_regexp;
    global $userquery;
    global $opterrs;
    global $basenodetypes;

    $q_colstr = '';
    foreach ($cols as $c) {
	if (array_key_exists($c,$colmap)) {
	    if (strlen($q_colstr) > 0) {
		$q_colstr .= ",";
	    }
	    $q_colstr .= $colmap[$c];
	}
    }

    # setup the main select/join string
    if (isset($experiment)) {
        $q_joinstr = " reserved as r" . 
            " left join nodes as n on r.node_id=n.node_id" .
	    " left join nodes as n2 on n.phys_nodeid=n2.phys_nodeid" . 
	    " left join node_types as nt2 on n2.type=nt2.type" . 
            " left join plab_mapping as pm on n.phys_nodeid=pm.node_id" . 
	    " left join plab_plc_info as ppi on pm.plc_idx=ppi.plc_idx";
    }
    elseif (isset($upnodefilter) && $upnodefilter == 'emulab') {
        # Note, the other half of this case (if $experiment isset AND this 
        # case) is covered later because it has to be postfiltered!
	$q_joinstr = " reserved as r" . 
            " left join nodes as n on r.node_id=n.node_id" .
	    " left join nodes as n2 on n.phys_nodeid=n2.phys_nodeid" . 
	    " left join node_types as nt2 on n.type=nt2.type" . 
            " left join plab_mapping as pm on n2.node_id=pm.node_id" . 
	    " left join plab_plc_info as ppi on pm.plc_idx=ppi.plc_idx";
    }
    else {
        $q_joinstr = " plab_mapping as pm";
        $q_joinstr .= " left join plab_plc_info as ppi" . 
	              " on pm.plc_idx=ppi.plc_idx";
    }
    # for now, just join all possible tables, even if we are not selecting data
    $q_joinstr .= " left join plab_site_mapping as psm on pm.node_id=psm.node_id";
    $q_joinstr .= " left join node_auxtypes as nat on pm.node_id=nat.node_id";
    $q_joinstr .= " left join node_status as ns on pm.node_id=ns.node_id";
    $q_joinstr .= " left join plab_comondata as pcd on pm.node_id=pcd.node_id";
    $q_joinstr .= " left join plab_nodehiststats as pnhs" . 
	" on pm.node_id=pnhs.node_id";
    #
    # Note that we have to use a subquery to get this crap.  It\'s either that
    # or use a view
    #
    $q_joinstr .= " left join (select pngm.node_id as node_id," . 
        " group_concat(png.name order by png.name) as nodegrouplist" . 
        " from plab_nodegroup_members as pngm" . 
        " left join plab_nodegroups as png" . 
        " on pngm.nodegroup_idx=png.nodegroup_idx" . 
        " group by pngm.node_id) as png on pm.node_id=png.node_id";

    # setup the quick filter string (note that all of these get anded)
    
    # note that all plab nodes get a non-"<nodetype_prefix>" node auxtype,
    # and this denotes to which network the node is connected.
    $rbtypes = array();
    foreach ($basenodetypes as $btype) {
        array_push($rbtypes,"'$btype'");
    }
    $q_quickfs = "!(nat.type in (".implode(',',$rbtypes)."))";
    if (isset($experiment)) {
        if (strlen($q_quickfs) > 0) {
	    $q_quickfs .= " and ";
        }
        $q_quickfs .= " r.pid='$pid' and r.eid='$eid'";
        $q_quickfs .= " and ";
        $q_quickfs .= " nt2.isplabphysnode=1";
    }
    if (isset($upnodefilter)) {
        if ($upnodefilter == "emulab") {
            if (strlen($q_quickfs) > 0) {
                $q_quickfs .= " and ";
            }
            $q_quickfs .= " ns.status='up'";
	    if (!isset($experiment)) {
		$q_quickfs .= " and nt2.isplabphysnode=1 and !(r.pid='emulab-ops' and r.eid='hwdown')";
	    }
        }
        elseif ($upnodefilter == "comon") {
            if (strlen($q_quickfs) > 0) {
                $q_quickfs .= " and ";
            }
            $q_quickfs .= " pcd.lastcotop < 305 and pcd.resptime > 0 " . 
		" and pcd.lastcotop is not null " . 
		" and (now() - from_unixtime(pcd.date)) < " . (12*60*60);
        }
    }
    if (isset($hostfilter) && $hf_regexp == 0) {
	if (strlen($q_quickfs) > 0) {
            $q_quickfs .= " and ";
        }
        $q_quickfs .= $colmap['hostname'] . " like '%$hostfilter%'";
    }

    # Setup the user filter string (can be very complex; is anded to the quick
    # filter string if it exists)
    $q_userfs = '';
    if (isset($userquery)) {
	$res = pm_parseuserquery($userquery,false);
	if (!$res[0]) {
	    array_push($opterrs,$res[1]);
	}
	else {
	    $q_userfs = $res[1];
	}
    }

    # finalize the filter string
    $q_finalfs = '';
    if (strlen($q_quickfs) > 0) {
        if (strlen($q_finalfs) > 0) {
	    $q_finalfs .= " and ";
        }
        $q_finalfs .= " $q_quickfs ";
    }
    if (strlen($q_userfs) > 0) {
        if (strlen($q_finalfs) > 0) {
            $q_finalfs .= " and ";
        }
        $q_finalfs .= " ($q_userfs) ";
    }
    if (strlen($q_finalfs) > 0) {
        $q_finalfs = "where $q_finalfs";
    }

    # setup sorting
    if (!isset($sortcols) || count($sortcols) == 0) {
        $sortcols = array('node_id');
    }
    $q_sortstr = 'order by ';
    foreach ($sortcols as $sc) {
        $q_sortstr .= $colmap_mysqlnames[$sc] . ",";
    }
    $q_sortstr = rtrim($q_sortstr,',');
    if (isset($sortdir)) {
        if ($sortdir != "asc" && $sortdir != "desc") {
            # XXX - bad sort param!
	    $sortdir = "asc";
        }
    }
    else {
        # we set one by default so that users do not have a hard time in the
        # default case
        $sortdir = "asc";
    }
    $q_sortstr .= " $sortdir";

    $q_groupstr = " ";

    # setup pagination
    $q_pagstr = '';
    if (!isset($limit)) {
        $limit = $DEF_LIMIT;
    }
    if (isset($limit) && $limit == 0) {
	$offset = 0;
    }
    if (isset($limit) && !isset($offset)) {
        $offset = 0;
    }
    if ($limit > 0) {
	$q_pagstr .= " limit $limit offset $offset";
    }

    return array($q_colstr,$q_joinstr,$q_finalfs,$q_sortstr,
		 $q_groupstr,$q_pagstr);
}

function pm_showtable($totalrows,$data) {
    global $sortcols,$sortdir,$colmap,$colmap_mysqlnames,$cols;
    global $colsrc,$colcol;
    global $selectable,$selectionlist,$newpgsel;

    $unicode_up = '&#x25b4;';
    $unicode_dn = '&#x25be;';

    echo "<center><div style=''>\n";
    echo "<p>\n";
    echo "<b>$totalrows nodes</b> matched your query.";
    echo "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;";
    foreach ($colcol as $k => $v) {
	echo "<span style='background-color: $v; border: 1px solid black;";
	echo "width: 16px; height: 16px;'>";
	echo "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;";
	echo "</span>\n";
	echo " $k ";
    }
    echo "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;";
    echo pm_getpaginationlinks($totalrows);
    echo "</p>\n";

    if (isset($selectable) && $selectable) {
	echo "<form name='nodesel' id='nodesel' action='plabmetrics.php3' method='get'" . 
	    " >\n";
        # Must ensure that the form field selectionlist is sent, so we can
        # whack it with javascript if necessary.
	$ova = array();
	if (!isset($selectionlist)) {
	    $ova = array('selectionlist' => '');
	}
	echo pm_gethiddenfields('nodesel',$ova);
	echo "<p>\n";
	echo "<input type='button' id='saveb' value='Save Selection'" . 
	    " onClick='this.form.submit()'>\n";
	echo "<input type='button' value='Select All on Page' " . 
	    " onClick='setSelectionAllCheckboxes(\"nodesel\",true)'>\n";
	echo "<input type='button' value='Unselect All on Page' " . 
	    " onClick='setSelectionAllCheckboxes(\"nodesel\",false)'>\n";
	echo "<input type='button' id='aisb' value='Select All in Search' " .
	    " onClick='setFormElementValue(\"nodesel\"," . 
	    "\"selectionlist\",\"ALLINSEARCH\");" . 
	    "this.form.submit()'>\n";
	echo "</p>\n";
    }

    # dump table header
    echo "<table>\n";
    echo "  <tr>\n";

    if (isset($selectable) && $selectable) {
	echo "<th valign='center'>Select</th>\n";
    }
    foreach ($cols as $c) {
        # reverse the sort direction if necessary
	$arr = '';
        if (isset($sortcols) && count($sortcols) == 1 
            && $sortcols[0] == $c) {
            $nsortdir = ($sortdir == "asc")?"desc":"asc";
	    if ($nsortdir == 'desc') {
		$arr = " " . $unicode_up;
	    }
	    else {
		$arr = " " . $unicode_dn;
	    }
        }
        else {
            $nsortdir = $sortdir;
	    if (in_array($c,$sortcols)) {
		if ($sortdir == 'desc') {
		    $arr = " " . $unicode_up;
		}
		else {
		    $arr = " " . $unicode_dn;
		}
	    }
	}
	$sstr = "background-color: " . $colcol[$colsrc[$c]];
        $url = pm_buildurl(array('sortcols' => $c,'sortdir' => $nsortdir));
        echo "    <th valign='center' style='$sstr'>" . 
	    "<a href='$url'>${c}${arr}</a></th>\n";
    }

    $tmppgsel = array();

    foreach ($data as $row) {
        echo "  <tr>\n";
	if (array_key_exists($colmap_mysqlnames['node_id'],$row) 
	    && isset($selectable) && $selectable) {
	    echo "<td>\n";
	    $checkedstr = '';
	    if (isset($selectionlist) 
		&& in_array($row[$colmap_mysqlnames['node_id']],
			    $selectionlist)) {
		array_push($tmppgsel,$row[$colmap_mysqlnames['node_id']]);
		$checkedstr = ' checked';
	    }
	    echo "<input type='checkbox' name='newpgsel[]' value='" . 
		$row[$colmap_mysqlnames['node_id']] . "'$checkedstr>\n";
	    echo "</td>\n";
	}
        foreach ($cols as $c) {
            $dbval = $row[$colmap_mysqlnames[$c]];
            # stupid mysql driver
            if (is_numeric($dbval) && is_float($dbval+0)) {
                $dbval = sprintf("%.4f",$dbval);
            }

	    if ($c == 'hostname') {
		echo "     <td><a href='http://$dbval:3120/cotop'>";
		echo "$dbval</a></td>";
	    }
            else {
		echo "     <td>$dbval</td>\n";
	    }
	}
    }
    foreach ($tmppgsel as $tval) {
	echo "<input type='hidden' name='pgsel[]' value='$tval'>\n";
    }

    echo "</table>\n";
    echo "<p>\n";
    echo pm_getpaginationlinks($totalrows);
    echo "</p>\n";

    if (isset($selectable) && $selectable) {
	echo "</form>";
    }

    echo "</div></center>\n";
}

#
# Parses a simple query expression.  Very dumb parser, but I do not want to 
# parse anything complex, so dumb works.  Basically, we look for 
# comparison operators (<,>,<=,>=,==,!=), boolean operators (and,or),
# names, constants, and containers (i.e., '(' or ')').  The rules are simple:
# open containers must always have a syntactically correct close container;
# each name must be followed by a comparison operator;
# each comparison must be followed by a constant;
# each boolean operator must be followed by a new container;
# containers may be implicit (that is, <name> <comp> <const> is a container).
#
function pm_parseuserquery($q,$debug = false) {
    global $colmap,$colmap_mysqlnames,$colmap_mysqlnames_where;

    # first tokenize
    $toka = array();
    # valid states are str,qstr,qqstr,cop,bop,''
    $tst = '';
    $tval = '';
    $i = 0;
    $errstr = '';
    foreach (range(0,strlen($q)-1) as $i) {
	$c = $q[$i];

	if ($tst == 'qqstr') {
	    if ($c == '\'') {
		$errstr = 'Quoted string cannot contain quotes.';
		break;
	    }
	    if ($c == '"') {
		array_push($toka,$tval);
		$tst = '';
		$tval = '';
	    }
	    else {
		$tval .= "$c";
	    }
	}
	elseif ($tst == 'qstr') {
	    if ($c == '"') {
		$errstr = 'Quoted string cannot contain quotes.';
		break;
	    }
	    if ($c == '\'') {
		array_push($toka,$tval);
		$tst = '';
		$tval = '';
	    }
	    else {
		$tval .= "$c";
	    }
	}
	elseif ($c == ' ') {
	    if ($tst == 'qstr' || $tst == 'qqstr') {
		$tval .= "$c";
	    }
	    elseif ($tst != '') {
		array_push($toka,$tval);
		$tval = '';
		$tst = '';
	    }
	}
	elseif ($c == '(' || $c == ')') {
	    if ($tst != '') {
		array_push($toka,$tval);
		$tst = '';
		$tval = '';
	    }
	    array_push($toka,$c);
	    $tval = '';
	    $tst = '';
	}
	elseif ($c == '\'') {
	    if ($tst != '') {
		array_push($toka,$tval);
	    }
	    $tst = 'qstr';
	    $tval = '';
	}
	elseif ($c == '"') {
	    if ($tst != '') {
		array_push($toka,$tval);
	    }
	    $tst = 'qqstr';
	    $tval = '';
	}
	elseif (ctype_alnum($c) || $c == '.' || $c == '-' || $c == '_') {
	    if ($tst == 'str' || $tst == '') {
		$tval .= "$c";
		$tst = 'str';
	    }
	    elseif ($tst != '') {
		array_push($toka,$tval);
		$tval = $c;
		$tst = 'str';
	    }
	    else {
		$errstr = "Invalid character '$c' in bareword at char $i!";
		break;
	    }
	}
	elseif ($c == '>' || $c == '<' || $c == '=' || $c == '!') {
	    if ($tst == 'cop') {
		$tval .= "$c";
	    }
	    elseif ($tst != '') {
		array_push($toka,$tval);
	    }
	    else {
		$tval = "$c";
		$tst = 'cop';
	    }
	}
	else {
	    $errstr = "Invalid character '$c' at char $i!";
	    break;
	}
    }
    # clean up last token
    if ($tval != '') {
	array_push($toka,$tval);
    }

    if (strlen($errstr) > 0) {
	return array(false,$errstr);
    }

    if ($debug) {
	echo "<br>tokens = '" . implode(',',$toka) . "'<br><br>\n";
    }
    

    # now apply the rules and generate some real sql:
    
    function isbop($tok) {
	$bops = array('and','or');
	foreach ($bops as $bop) {
	    if ($bop == $tok)
		return true;
	}
	return false;
    }
    function iscop($tok) {
	$cops = array('<','>','<=','>=','==','!=','like');
	foreach ($cops as $cop) {
	    if ($cop == $tok)
		return true;
	}
	return false;
    }
    function isname($tok) {
	global $colmap;
	foreach (array_keys($colmap) as $col) {
	    if ($col == $tok)
		return true;
	}
	return false;
    }
    function isnum($tok) {
	if (preg_match("/^-{0,1}\d+(\.\d+){0,1}$/",$tok) == 1) {
	    return true;
	}
	return false;
    }
    function isstr($tok) {
	if (preg_match("/^[\w\d\s\.\-_:]+$/",$tok) == 1) {
	    return true;
	}
	return false;
    }
    function sat($tok,$need) {
	if ($need == 'name' && isname($tok)) {
	    return true;
	}
	elseif ($need == 'cop' && iscop($tok)) {
	    return true;
	}
	elseif ($need == 'bop' && isbop($tok)) {
	    return true;
	}
	elseif ($need == 'const' && (isnum($tok) || isstr($tok))) {
	    return true;
	}
	elseif ($need == 'ocont' && $tok == '(') {
	    return true;
	}
	elseif ($need == 'ccont' && $tok == ')') {
	    return true;
	}
	return false;
    }
    function whichsat($tok,$lneeds) {
	foreach ($lneeds as $n) {
	    if (sat($tok,$n)) {
		return $n;
	    }
	}
	return '';
    }
	

    $retq = '';
    $oc = $cc = 0;
    
    # We have to adjust the parser state transitions if we are ahead or 
    # following a comparison operator, so that we can properly look for
    # either a boolean op or close paren.
    $pre_cop_name_const_trans = array('cop');
    $post_cop_name_const_trans = array('ccont','bop');
    
    $trans = array( 'name' => $pre_cop_name_const_trans,
		    'ocont' => array('ocont','name','const'),
		    'ccont' => array('ccont','bop'),
		    'const' => $pre_cop_name_const_trans,
		    'cop' => array('name','const'),
		    'bop' => array('ocont','name','const') );
    
    # Initial transition.
    $needs = array('name','const','ocont');

    # We have to keep track of if we have seen the cop, or have not, 
    # in an <name|const> <cop> <name|const> expr, so that we know which
    # needs are valid.
    $sawcop = false;

    $prev_toka_like = false;
    foreach ($toka as $i) {
        #echo "needs = " . implode(',',$needs) . "; <br>\n";
	$sneed = whichsat($i,$needs);
	if ($sneed == '') {
	    $errstr = "Parser error at token '$i'!";
	    break;
	}

	$just_set_like = false;

	$needs = $trans[$sneed];
	if (!isset($needs)) {
	    $needs = array();
	}

        #echo "sneed = $sneed; newneeds = " . implode(',',$needs) . "<br>\n";

	if ($sneed == 'ocont') {
	    ++$oc;
	    $retq .= '(';
	}
	elseif ($sneed == 'ccont') {
	    ++$cc;
	    $retq .= ')';
	}
        # XXX: handle the problem caused by use of ' as ' in colmap
	elseif ($sneed == 'name') {
	    $retq .= " " . $colmap_mysqlnames_where[$i] . " ";
	    if ($sawcop) {
		$sawcop = false;
		$trans['name'] = $pre_cop_name_const_trans;
		$trans['const'] = $pre_cop_name_const_trans;
	    }
	}
	elseif ($sneed == 'const') {
	    $cstr = '';
	    if (isnum($i)) {
		$cstr = $i;
	    }
	    else {
                # add the wildcards on user behalf
		$ss = '';
		if ($prev_toka_like) {
		    $ss = '%';
		}
		$cstr = "'$ss" . $i . "$ss'";
	    }
	    $retq .= " $cstr ";
	    if ($sawcop) {
		$sawcop = false;
		$trans['name'] = $pre_cop_name_const_trans;
		$trans['const'] = $pre_cop_name_const_trans;
	    }
	}
	elseif ($sneed == 'cop') {
	    if ($i == '==') {
		$retq .= ' = ';
	    }
	    else {
		$retq .= " $i ";
	    }
	    $sawcop = true;
	    $trans['name'] = $post_cop_name_const_trans;
	    $trans['const'] = $post_cop_name_const_trans;

	    if ($i == "like") {
		$prev_toka_like = true;
		$just_set_like = true;
	    }
	}
	elseif ($sneed == 'bop') {
	    $retq .= " $i ";
	}

        # clear special case for like cop
	if (!$just_set_like) {
	    $prev_toka_like = false;
	}
    }
    if (strlen($errstr) > 0) {
	return array(false,$errstr);
    }

    if ($debug) {
	echo "user query as SQL = '$retq' !<br>\n";
    }

    return array(true,$retq);
}

?>
