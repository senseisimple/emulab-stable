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

#
# Standard Testbed Header
#
# cheat and use the page args to influence the view.
$view = array();
if (isset($_REQUEST['pagelayout']) && $_REQUEST['pagelayout'] == "minimal") {
    $view = array ('hide_banner' => 1, 'hide_sidebar' => 1 );
}

PAGEHEADER("PlanetLab Metrics",$view);


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

function divflipvis(id) {
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
	obj.style.position = "static";
	divstate[id].vis = obj.style.visibility = "visible";
    }
}

function init() {
    divinit('searchdiv');
    divinit('legenddiv');
    divinit('selectiondiv');

    divflipvis('searchdiv');
    divflipvis('legenddiv');
    divflipvis('selectiondiv');
}

window.onload = init;

function setFormElementValue(id,value) {
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

</script>
<?php

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

#
# hack -- unset $pid and $eid vars if they are '' before we check args!
# (needed so we can let users drop a previous pid/eid filter)
#
if (isset($_REQUEST['pid']) && $_REQUEST['pid'] == '') {
    unset($_REQUEST['pid']);
}
if (isset($_REQUEST['eid']) && $_REQUEST['eid'] == '') {
    unset($_REQUEST['eid']);
}

# same for upnodefilter arg
if (isset($_REQUEST['upnodefilter']) && $_REQUEST['upnodefilter'] == '') {
    unset($_REQUEST['upnodefilter']);
}

# make sure to add any new args here to pm_buildurl() as well
$optargs = OptionalPageArguments( "experiment", PAGEARG_EXPERIMENT,
				  "cols",   PAGEARG_ALPHALIST,
				  "offset", PAGEARG_INTEGER,
				  "limit",  PAGEARG_INTEGER,
				  "upnodefilter", PAGEARG_STRING,
				  "pagelayout", PAGEARG_BOOLEAN,
				  "sortcols", PAGEARG_ALPHALIST,
				  "sortdir", PAGEARG_STRING,
                                  # never fear -- we check this ourself since
                                  # it could contain either an alphanumeric
                                  # string, or a perl5 regex!
				  # (only alphanum strings go in db queries!)
				  "hostfilter", PAGEARG_ANYTHING,
				  "selectmode", PAGEARG_BOOLEAN,
				  "selectionlist", PAGEARG_ALPHALIST,
                                  # we check this *extremely* copiously.
                                  # it actually gets tokenized and we verify
                                  # every last token against known or 
                                  # numeric values
				  "userquery", PAGEARG_ANYTHING,
                                  # verified, but never leaves php
				  "colrank", PAGEARG_ANYTHING);

# link to show/hide sidebar

# basic options
#   hostname filter (basically, text to a like "%<text>%" sql expr)
#   enable/disable selection; show current selection and be able to remove 
#     nodes from it!

# advanced options
#   constrain columns
#     some sort of query language, sigh -- maybe can find a php lib for it
#   build-your-own rank (based off column weights, normalization to max or 
#     user-chosen ceil; each col should be able to be minimized/maximized in 
#     its contribution to the rank)

# should provide cotop links (one node-centric, one slice-centric)
# paint comon columns red, emulab green, or something like that.


# A map of colnames the user can refer to, to their name in queries we issue.
$colmap = array( 'node_id' => 'pm.node_id',
		 'hostname' => 'pm.hostname',
		 'plab_id' => 'pm.plab_id',
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
		 'rxrate' => 'pcd.rxrate',
                 # emulab columns
		 'unavail' => 'pnhs.unavail',
		 'jitdeduction' => 'pnhs.jitdeduct',
		 'successes' => 'pnhs.succnum',
		 'failures' => 'pnhs.failnum',
		 'nodestatus' => 'ns.status',
		 'nodestatustime' => 'ns.status_timestamp',
                 # what about what kind of link the node contains 
		 # (i.e., inet,inet2,dsl,...) ?
	       );

#
# First, deal with args:
#

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
    array_push($opterrs,"Only hostname characters are supported for" . 
	                " hostname filtering.");
    unset($hostfilter);
    $hf_regexp = 1;
}

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
# We want to find all pid/eids that have mapped plab nodes that we can read.
if ($isadmin) {
    $pequery = "
select e.pid,e.eid from experiments as e 
left join reserved as r on e.pid=r.pid and e.eid=r.eid 
left join nodes as n on r.node_id=n.node_id 
left join node_types as nt on n.type=nt.type 
where nt.type='pcplab' group by e.pid,e.eid";
}
else {
    $pequery = "
select e.pid,e.eid from group_membership as g 
left join experiments as e on g.pid=e.pid and g.pid=g.gid 
left join reserved as r on e.pid=r.pid and e.eid=r.eid 
left join nodes as n on r.node_id=n.node_id 
left join node_types as nt on n.type=nt.type 
where g.uid='$uid' and e.pid is not null and e.eid is not null 
  and nt.type='pcplab' group by e.pid,e.eid";
}
$qres = DBQueryFatal($pequery);

$pidmap = array();

if (mysql_num_rows($qres) == 0) {
    TBERROR("Unexpected number of rows in count query!");
}
else {
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

?>

</script>

<?php

# we have to fix which node id we select based on if we are querying the 
# reserved table (we need phys node id, but have to join reserved to nodes
# to get it)
if (isset($experiment)) {
    $colmap['node_id'] = 'n.phys_nodeid';
}

# strip off the table prefixes or use 'as' aliases; mysql driver does not 
# seem to include them
$colmap_mysqlnames = array();
foreach ($colmap as $k => $v) {
    if (strstr($v,' as ')) {
	$sa = explode(' as ',$v);
	$nv = $sa[1];
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
}

# default columns displayed, in this order.
# note that they cannot get rid of node_id.
$defcols = array( 'hostname','nodestatus','nodestatustime','unavail',
		  '5minload','freemem','txrate','rxrate','date', );

# default sort and included columns
$defsortcols = array( 'unavail','5minload' );

if (!isset($cols) || count($cols) == 0) {
    $cols = $defcols;
}

if (!isset($sortcols)) {
    $sortcols = $defsortcols;
    $sortdir = 'asc';
}

# make sure all cols are valid!  remove them if they are not!
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

# make sure sortdir is valid (asc/desc):
if (isset($sortdir) && $sortdir != 'asc' && $sortdir != 'desc') {
    array_push($opterrs,"Invalid sort dir '$sortdir'; changing to 'asc'!");
    $sortdir = 'asc';
}

#
# Next, build query:
#

# Grab query parts
$qbits = pm_buildqueryinfo();
$select_s = $qbits[0]; $src_s = $qbits[1]; $filter_s = $qbits[2]; 
$sort_s = $qbits[3]; $pag_s = $qbits[4];

# query that counts num data rows
$qcount = "select count(" . $colmap['node_id'] . ") as num" . 
    " from $src_s $filter_s";

# query that gets data
$q = "select $select_s from $src_s $filter_s $sort_s $pag_s";

#echo "qcount = $qcount<br><br>\n";
#echo "q = $q<br><br>\n";

$qres = DBQueryFatal($qcount);
if (mysql_num_rows($qres) != 1) {
    TBERROR("Unexpected number of rows in count query!");
}
$row = mysql_fetch_array($qres);
$totalrows = $row['num'];

#
# Draw nondata part of page
#
pm_shownondata();

echo "<br><br>";

#
# Finally, display data:
#
pm_showtable($totalrows,$q);

#
# Standard Testbed Footer
#
PAGEFOOTER();

#
# Show the non-data part of the page:
#
function pm_shownondata() {
    global $pagelayout;
    global $opterrs;

    echo "<table class='stealth'>\n";
    echo "<tr>\n";
    echo "<td class='stealth' valign='top' width='1px'>\n";
    SUBMENUSTART("Page Options");
    WRITESUBMENUBUTTON("Search Options",
		       "javascript:divflipvis(\"searchdiv\")");
    WRITESUBMENUBUTTON("Selected Nodes",
		       "javascript:divflipvis(\"selectiondiv\")");
    WRITESUBMENUBUTTON("Data Legend",
		       "javascript:divflipvis(\"legenddiv\")");
    WRITESUBMENUDIVIDER();
    $url = pm_buildurl(array('pagelayout' => 'minimal'));
    WRITESUBMENUBUTTON("Hide Sidebar",$url);
    SUBMENUEND();
    echo "</td>\n";
    echo "<td class='stealth'>\n";
    if (count($opterrs) > 0) {
	echo "<span style='color: red'>Errors:<br>\n";
	foreach ($opterrs as $e) {
	    echo "&nbsp;&nbsp;$e<br>\n";
	}
	echo "</span><br>\n";
    }
    pm_showsearch();
    echo "<br>\n";
    pm_showlegend();
    echo "<br>\n";
    pm_showselection();
    echo "</td>\n";
    echo "</tr>\n";
    echo "</table>\n";
}

#
# Show the selection box
#
function pm_showselection() {
    echo "<div style='padding: 8px; border: 2px dashed gray; visibility: visible; width: 100%' id='selectiondiv'>\n";
    echo "Selection not yet enabled!\n";
    echo "</div>\n";
}

#
# Show the legend box
#
function pm_showlegend() {
    echo "<div style='padding: 8px; border: 2px dashed gray; visibility: visible' id='legenddiv'>\n";
    echo "Coming soon.  For now, look at the " . 
	"<a href='http://summer.cs.princeton.edu/status/legend.html'>CoMon " .
	"legend</a>.\n";
    echo "</div>\n";
}

#
# Show the search box.
#
function pm_showsearch() {
    global $limit,$DEF_LIMIT,$offset;
    global $upnodefilter,$pid,$eid,$sortcols,$cols,$sortdir;
    global $pidmap,$colmap;
    global $hostfilter;

    echo "<div style='margin-right: auto; padding: 8px; border: 2px dashed gray; visibility: visible' id='searchdiv'>\n";
    echo "<form name='plsearchform' id='plsearchform'" . 
	" action='plabmetrics.php3' method='post' " . 
        # We manually set a comma-delineated string comprised of the col names
        # in the display cols select box.  This way, we still only must handle
        # the cols var, instead of both the cols and selcols[] vars on 
        # search box post vs url get.
        " onSubmit='setFormElementValue(\"cols\",".
	"dynselect.getOptsAsDelimString(document.getElementById(\"selcols\"),".
	"\",\",false)); " . 
	"setFormElementValue(\"sortcols\",".
	"dynselect.getOptsAsDelimString(document.getElementById(\"selsortcols\"),".
	"\",\",false));" . "'" . ">\n";

    
    # only draw pid/exp boxes if the user can access experiments with pl nodes
    if (count($pidmap)) {
	echo "Show <b>only nodes in project/experiment</b>:\n";
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
    echo "Show <b>only nodes considered alive</b> by ";
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
    echo "Show only hostnames that match ";
    $thf = '';
    if (isset($hostfilter)) {
	$thf = $hostfilter;
    }
    echo "<input type='text' value='$thf' name='hostfilter' size='25'>\n";
    echo "<br>\n";

    # handle limit/offset
    $defLimits = array( '10' => 10,'20' => 20,'50' => 50,
		        '100' => 100,'All' => 0 );
    echo "Show ";
    echo "<select name='limit' id='limit'";
    # XXX
    # if the user changes the limit we change the offset to 0 so they
    # do not get funky page offset stuff
    echo "  onChange='setFormElementValue(\"offset\",0)'>\n";
    foreach ($defLimits as $limk => $limv) {
	echo "  <option value='$limv' ";
	if ($limv == $limit) {
	    echo " selected";
	}
	echo ">$limk</option>\n";
    }
    echo "</select>\n";
    echo " PlanetLab <b>nodes per page</b>\n";
    echo "<input type='hidden' name='offset' id='offset' value='$offset'>\n";
    echo "<br><br>\n";


    # hidden vals for col selection and sorting
    echo "<input type='hidden' name='cols' id='cols' value='$cols'>\n";
    echo "<input type='hidden' name='sortcols' id='sortcols' " . 
	"value='$sortcols'>\n";


    # choose which columns are shown and what the column order should be
    # first do the headers -- col order and col sort get munged here!
    echo "<table class='stealth'>\n";
    echo "<tr>\n";
    echo "<td class='stealth' colspan='4'>\n";
    echo "<b>Show and reorder</b> columns:<br>\n";
    echo "</td>\n";
    # space
    echo "<td class='stealth' colspan='4'>\n";
    echo "&nbsp;&nbsp;&nbsp;&nbsp;";
    echo "</td>\n";
    echo "<td class='stealth' colspan='4'>\n";
    echo "<b>Sort selected</b> columns:\n";
    echo "</td>\n";
    echo "</tr>\n";
    
    echo "<tr>\n"; 
    echo "<td class='stealth'>\n";
    echo "<select name='acols' id='acols' multiple size='6'>\n";
    # Compute the diff between avail cols and the current col selection.
    # avail cols go in the left box, current shown cols in the right.
    $da = array_diff(array_keys($colmap),$cols);
    foreach ($da as $foo) {
	echo "  <option value='$foo'>$foo</option>\n";
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
	echo "  <option value='$foo'>$foo</option>\n";
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
	echo "  <option value='$foo'>$foo</option>\n";
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
	echo "  <option value='$foo'>$foo</option>\n";
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
    echo "<br>\n";

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
    global $offset,$limit,$upnodefilter,$pagelayout,$pid,$eid;
    global $sortcols,$sortdir;

    $pageargs = array( 'pid','eid','cols','sortcols','sortdir',
                       'upnodefilter','pagelayout','limit','offset',
		       'hostfilter','userquery' );

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
            $v = implode($v,',');
        }
        $retval .= "$k=" . urlencode($v) . "&";
    }
    $retval = rtrim($retval,'&');

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
    global $limit,$offset;
    global $pid,$eid;
    global $DEF_LIMIT;
    global $hostfilter,$hf_regexp;

    $q_colstr = $colmap['node_id'];
    foreach ($cols as $c) {
	if (array_key_exists($c,$colmap)) {
	    $q_colstr .= "," . $colmap[$c];
	}
    }

    # setup the main select/join string
    if (isset($experiment)) {
        $q_joinstr = " reserved as r" . 
            " left join nodes as n on r.node_id=n.node_id" .
            " left join plab_mapping as pm on n.phys_nodeid=pm.node_id";
    }
    else {
        $q_joinstr = " plab_mapping as pm";
    }
    # for now, just join all possible tables, even if we are not selecting data
    $q_joinstr .= " left join node_status as ns on pm.node_id=ns.node_id";
    $q_joinstr .= " left join plab_comondata as pcd on pm.node_id=pcd.node_id";
    $q_joinstr .= " left join plab_nodehiststats as pnhs" . 
	" on pm.node_id=pnhs.node_id";

    # setup the quick filter string (note that all of these get anded)
    $q_quickfs = '';
    if (isset($experiment)) {
        if (strlen($q_quickfs) > 0) {
	    $q_quickfs .= " and ";
        }
        $q_quickfs .= " r.pid='$pid' and r.eid='$eid'";
    }
    if (isset($upnodefilter)) {
        if ($upnodefilter == "emulab") {
            if (strlen($q_quickfs) > 0) {
                $q_quickfs .= " and ";
            }
            $q_quickfs .= " ns.status='up'";
        }
        elseif ($upnodefilter == "comon") {
            if (strlen($q_quickfs) > 0) {
                $q_quickfs .= " and ";
            }
            $q_quickfs .= " (pcd.lastcotop - now()) < 600" . 
		" and pcd.lastcotop is not null";
        }
    }
    if (isset($hostfilter) && $hf_regexp == 0) {
	if (strlen($q_quickfs) > 0) {
            $q_quickfs .= " and ";
        }
        $q_quickfs .= $colmap['hostname'] . " like '%$hostfilter%'";
    }

    # setup the user filter string (can be very complex; is anded to the quick
    # filter string if it exists)
    $q_userfs = '';

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
        $q_finalfs .= " $q_userfs ";
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
        $q_sortstr .= $colmap[$sc] . ",";
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

    return array($q_colstr,$q_joinstr,$q_finalfs,$q_sortstr,$q_pagstr);
}

function pm_showtable($totalrows,$query) {
    global $sortcols,$sortdir,$colmap,$colmap_mysqlnames,$cols;

    echo "<center><div style=''>\n";
    echo pm_getpaginationlinks($totalrows);
    echo "<br>\n";

    # dump table header
    echo "<table>\n";
    echo "  <tr>\n";

    # reverse the sort direction if necessary
    if (isset($sortcols) && count($sortcols) == 1 
        && $sortcols[0] == 'node_id') {
        $nsortdir = ($sortdir == "asc")?"desc":"asc";
    }
    else {
        $nsortdir = $sortdir;
    }
    $url = pm_buildurl(array('sortcols' => 'node_id','sortdir' => $nsortdir));
    echo "    <th><a href='$url'>node_id</a></th>\n";
    foreach ($cols as $c) {
        # reverse the sort direction if necessary
        if (isset($sortcols) && count($sortcols) == 1 
            && $sortcols[0] == $c) {
            $nsortdir = ($sortdir == "asc")?"desc":"asc";
        }
        else {
            $nsortdir = $sortdir;
        }
        $url = pm_buildurl(array('sortcols' => $c,'sortdir' => $nsortdir));
        echo "    <th><a href='$url'>$c</a></th>\n";
    }

    # now run the real query
    $qres = DBQueryFatal($query);

    while ($row = mysql_fetch_array($qres)) {
        echo "  <tr>\n";
        echo "    <td>" . $row[$colmap_mysqlnames['node_id']] . "</td>\n";
        foreach ($cols as $c) {
            $dbval = $row[$colmap_mysqlnames[$c]];
            # stupid mysql driver
            if (is_numeric($dbval) && is_float($dbval+0)) {
                $dbval = sprintf("%.4f",$dbval);
            }
            echo "      <td>$dbval</td>\n";
        }
    }

    echo "</table>\n";
    echo pm_getpaginationlinks($totalrows);
    echo "</div></center>\n";
}

?>
