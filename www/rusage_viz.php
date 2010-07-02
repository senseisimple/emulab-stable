<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2008 University of Utah and the Flux Group.
# All rights reserved.
#
require("defs.php3");

#
#
# Only known and logged in users allowed.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify Page Arguments.
#
$optargs = OptionalPageArguments("resource", PAGEARG_STRING,
				 "getdata", PAGEARG_BOOLEAN);

# if you change any of the resource queries, you must change these arrays!
$resources = array("project","experiment","user","nodetype");
$fields = array();
$fields["project"] = array("pcs" => array("type" => "size","data" => "count"),
			   "exps" => array("type" => "color",
					   "data" => "count"));
$fields["experiment"] = array("pcs" => array("type" => "size",
					     "data" => "count"),
			      "idle" => array("type" => "color",
					      "data" => "time"),
#"stale" => array("type" => "color",
#"data" => "time"),
			      "duration" => array("type" => "color",
						  "data" => "time"));
$fields["user"] = array("pcs" => array("type" => "size","data" => "count"),
			"exps" => array("type" => "color","data" => "count"));

$fields["nodetype"] = array("experiment" => array("type" => "multi",
						  "data" => "text"),
			    "pcs" => array("type" => "size","data" => "count"),
			    "idle" => array("type" => "color",
					    "data" => "time"),
			    "duration" => array("type" => "color",
						"data" => "time"));

$defResource = "experiment";
$defColorField = "idle";

# resource queries
$queries = array();

$queries["project"] = "select r.pid as project,count(r.node_id) as pcs," . 
    "  count(distinct r.eid) as exps" . 
    "  from reserved as r" . 
    " left join nodes as n on r.node_id=n.node_id" . 
    " left join node_types as nt on n.type=nt.type" . 
    " where nt.class='pc'" . 
    "  and !(r.pid='emulab-ops' and (r.eid='wifi-holding' or r.eid='hwdown'" . 
    "    or r.eid='oddball' or r.eid='reloading'))" .
    " group by r.pid order by pcs desc";

$queries["experiment"] = "select concat_ws(' / ',r.pid,r.eid) as experiment," .
    "  count(r.node_id) as pcs," .
    "  min(unix_timestamp(now()) - unix_timestamp(greatest(na.last_tty_act,na.last_net_act,na.last_cpu_act,na.last_ext_act))) as idle," . 
#"  (unix_timestamp(now()) - unix_timestamp(na.last_report)) as stale," . 
    "  max(unix_timestamp(now()) - unix_timestamp(e.expt_swapped)) as duration" . 
    " from reserved as r" . 
    " left join nodes as n on r.node_id=n.node_id" . 
    " left join node_types as nt on n.type=nt.type" . 
    " left join node_activity as na on r.node_id=na.node_id" . 
    " left join experiments as e on (r.pid=e.pid and r.eid=e.eid)" . 
    " where nt.class='pc'" . 
    "  and !(r.pid='emulab-ops' and (r.eid='wifi-holding' or r.eid='hwdown'" . 
    "    or r.eid='oddball' or r.eid='opsnodes' or r.eid='plab-monitor'" . 
    "    or r.eid='plab-testing' or r.eid='plabnodes' or r.eid='reloading'))" .
    " group by r.pid,r.eid order by pcs desc";

$queries["user"] = "select u.uid as user,count(r.node_id) as pcs," . 
    "  count(distinct e.eid) as exps" . 
    " from reserved as r" . 
    " left join nodes as n on r.node_id=n.node_id" . 
    " left join node_types as nt on n.type=nt.type" . 
    " left join experiments as e on (r.pid=e.pid and r.eid=e.eid)" . 
    " left join users as u on e.swapper_idx=u.uid_idx" . 
    " where nt.class='pc'" . 
    "  and !(r.pid='emulab-ops' and (r.eid='wifi-holding' or r.eid='hwdown'" . 
    "    or r.eid='oddball' or r.eid='opsnodes' or r.eid='plab-monitor'" . 
    "    or r.eid='plab-testing' or r.eid='plabnodes' or r.eid='reloading'))" .
    " group by (u.uid) order by pcs desc";

$queries["nodetype"] = "select n.type as nodetype," . 
    "  concat_ws(' / ',r.pid,r.eid) as experiment," . 
    "  count(distinct r.node_id) as pcs," . 
    "  min(unix_timestamp(now()) - unix_timestamp(greatest(na.last_tty_act,na.last_net_act,na.last_cpu_act,na.last_ext_act))) as idle," . 
    "  (unix_timestamp(now()) - unix_timestamp(e.expt_swapped)) as duration" . 
    " from reserved as r" . 
    " left join experiments as e on (r.pid=e.pid and r.eid=e.eid)" . 
    " left join nodes as n on r.node_id=n.node_id" . 
    " left join node_types as nt on n.type=nt.type" . 
    " left join node_activity as na on r.node_id=na.node_id" . 
    " where nt.class='pc'" . 
    " group by n.type,r.pid,r.eid order by n.type,pcs desc";

if (isset($getdata) && $getdata) {
    # spit a few headers
    header('Content-type: text/html; charset=utf-8');
    header("Last-Modified: " . gmdate("D, d M Y H:i:s") . " GMT");
    header("Cache-Control: no-cache, must-revalidate");
    header("Pragma: no-cache");

    if (!$isadmin) {
	echo("unauthorized\n");
	return;
    }

    # check args
    if (!isset($resource)) {
	echo("no resource specified\n");
	return;
    }
    if (!in_array($resource,$resources)) {
	echo("invalid resource\n");
	return;
    }

    # grab the data
    $qres = DBQueryFatal($queries[$resource]);
    if (mysql_num_rows($qres) == 0) {
	echo("no data\n");
	return;
    }

    # print column headers
    echo "$resource";

    foreach ($fields[$resource] as $f => $finfo) {
	echo "\t$f(".$finfo["type"].")(".$finfo["data"].")";
    }
    echo "\n";

    # print data
    while ($row = mysql_fetch_array($qres)) {
	echo $row[$resource];
	foreach ($fields[$resource] as $f => $finfo) {
	    echo "\t" . $row[$f] . "";
	}
	echo "\n";
    }

    return;
}

#
# Standard Testbed Header (do this after checking getdata so we do not spit 
# headers if getdata=1 -- of course, if any args are bad, the error msgs will
# be screwed up!)
#
PAGEHEADER("Resource Usage Visualization");

#
# Pull in some style sheets.
#
echo "<link type=text/css rel=stylesheet href=${TBBASE}/rusage_viz.css />\n";

if (! $isadmin) {
    USERERROR("You do not have permission to view the resource usage viz!", 1);
}

#
# Dump some Emulab auth vars for the xmlhttprequests
#
$auth = $HTTP_COOKIE_VARS[$TBAUTHCOOKIE];
echo "<script language=javascript type=text/javascript>\n";
echo "var tbuid = \"$uid\";\n";
echo "var tbauth = \"$auth\";\n";

#
# Dump the resource, if set
#
if (isset($resource)) {
    echo "var resource = \"$resource\";\n";
}
else {
    echo "var resource = \"$defResource\";\n";
}

#
# Dump the default coloring field.
# 
if (isset($resource)) {
    $color = "null";
    foreach ($fields[$resource] as $f => $finfo) {
	if ($finfo["type"] == "color") {
	    $color = $f;
	    break;
	}
    }
    echo "var defaultColor = \"$f\";\n";
    echo "var currentColor = \"$f\";\n";
}
else {
    echo "var defaultColor = \"$defColorField\";\n";
    echo "var currentColor = \"$defColorField\";\n";
}

echo "var TBBASE = \"$TBBASE\";";

echo "</script>\n\n";

#
# add some other javascripts
#
echo "<script language=javascript type=text/javascript src=${TBBASE}/js/mootools-core.js></script>";
echo "<script language=javascript type=text/javascript src=${TBBASE}/js/mootools-tips.js></script>";
echo "<script language=javascript type=text/javascript src=${TBBASE}/js/treemap.js></script>";

#
# An inline bit...
#
?>
<script language=javascript type=text/javascript>

/*
 * Do we need debug output?
 */
var dodebug = false;

/*
 * The Treemap object.
 */
var tm = null;

function error(msg) {
    $('errorbox').innerHTML = "<b>ERROR</b> in data retrieval:<br/><pre>" + msg + 
	"</pre><br/>";
}

function debug(msg) {
    if (dodebug) {
	$('errorbox').innerHTML += "<b>INFO</b>: " + msg + "<br/>";
    }
}

function parseDatum(datum,type) {
    if (type == "count") {
	return [parseInt(datum),null];
    } 
    else if (type == "time") {
	var n = parseInt(datum);
	var labeled = n + "s";
	if (n > 24*3600) {
	    labeled = new Number(n / (24*3600)).toFixed(1) + "d";
	}
	else if (n > 3600) {
	    labeled = new Number(n / 3600).toFixed(1) + "h";
	}
	else if (n > 60) {
	    labeled = new Number(n / 60).toFixed(1) + "m";
	}
	/*
	 * Note that we do something very nasty -- set a max time!
	 * Do this so that the effect of the colors doesn't get lost.
	 */
	var MONTH = 31*24*3600;
	var WEEK = 7*24*3600;
	var DAY = 24*3600;
	if (n > DAY) {
	    n = DAY;
	}
	return [n,labeled];
    }
    else {
	return null;
    }
}

/*
 * Simple function to process simple data text format and generate an array of
 * big JSON-like objects for the JIT treemap.  We generate an object for each
 * "colorable" field so the user can hot-swap the trees.  So, the return data
 * looks like:
 *   
 *   "objects":{"field1":tree1,...}
 *   "fields":["field1",...]
 *   "fieldtypes":{"field1":"type1",...}
 *   "fielddatatypes":{"field1":"datatype1",...}
 *   
 *   
 *   

 */
function parseAndBuild(data) {
    var retval = {};
    var lines = data.split("\n");
    if (lines.length < 3) {
	// there was an error
	if (lines.length == 0) {
	    error("no data in response from server");
	}
	else if (lines.length > 0) {
	    error(lines[0]);
	}
	return null;
    }

    // handle header line first
    var hdr_pat = /([\w_\-]+)\(([\w_\-]+)\)\(([\w_\-]+)\)/;
    var fields = lines[0].split("\t");
    var presource = fields[0];
    var hdr = [fields[0]];
    var hdr_info = {};
    var num_multi = 0;
    var objects = {};
    retval["id"] = retval["name"] = presource;
    for (var i = 1; i < fields.length; ++i) {
	var ma = fields[i].match(hdr_pat);
	if (!ma) {
	    error("could not parse " + fields[i]);
	    return null;
	}
	hdr[i] = ma[1];
	hdr_info[ma[1]] = {"type":ma[2],"datatype":ma[3]};
	// yes, very wasteful of memory, but this way we don't change the 
	// treemap lib
	if (ma[2] == "color") {
	    objects[ma[1]] = { "children":[],"id":presource,"name":presource };
	}
	else if (ma[2] == "multi") {
	    ++num_multi;
	}
    }

    var debugstr = "";

    var size_key = "";
    var ccount = 0;
    var colorvals = {};
    for (var i = 1; i < lines.length; ++i) {
	if (lines[i] == "") {
	    continue;
	}
	var da = lines[i].split("\t");
	debugstr += da.toString() + "<br/>";
	var cfcount = 0;
	for (var colorfield in objects) {
	    var k = 2;
	    var ca = {};
	    if (colorvals[colorfield] == undefined) {
		colorvals[colorfield] = { "max":Number.MIN_VALUE,
					  "min":Number.MAX_VALUE }
	    }
	    // handle first field specially (it's the name of the resource)
	    ca["id"] = ca["name"] = da[0];
	    ca["data"] = [];
	    debugstr += "da.length = " + da.length + "<br/>";

	    /* 
	     * We start at the first non-multi column (after the main key
	     * column, that is).
	     */
	    for (var j = num_multi + 1; j < da.length; ++j) {
		vals = parseDatum(da[j],hdr_info[hdr[j]]["datatype"]);
		var iidx = null;
		if (hdr[j] == colorfield) {
		    iidx = 1;
		    // figure out max/min for this colorfield.
		    if (vals[0] > colorvals[colorfield]["max"]) {
			colorvals[colorfield]["max"] = vals[0];
		    }
		    if (vals[0] < colorvals[colorfield]["min"]) {
			colorvals[colorfield]["min"] = vals[0];
		    }
		}
		else if (hdr_info[hdr[j]]["type"] == "size") {
		    iidx = 0;
		    // we only count for the first colorfield
		    if (!cfcount) {
			size_key = hdr[j];
		    }
		}
		else {
		    iidx = k++;
		}
		// only set value if this is a leaf.
		ca["data"][iidx] = { "value":vals[0],"key":hdr[j] };
		if (vals[1]) {
		    ca["data"][iidx]["label_value"] = vals[1];
		}
		debugstr += "added at idx = " + iidx + "<br/>";
	    }
	    debugstr += "data.length = " + ca["data"].length + "<br/>";
	    ca["children"] = [];
	    /*
	     * Now, the tricky part -- we have to figure out where in the
	     * hierarchy to insert the `ca' object we just created:
	     */
	    if (num_multi > 0) {
		var insertTarget = objects[colorfield]["children"];
		// note that we don't go through all the multi 
		// columns -- we go through n-1 and then handle the nth
		// after the loop!  Essentially, we whack the ca.{id,name}
		// fields created above and set them to the da[num_multi-1]
		// value, since they're wrong for the num_multi > 0 case.
		for (var lpc = 0; lpc < (num_multi); ++lpc) {
		    var found = false;
		    for (var j = 0; j < insertTarget.length; ++j) {
			// look for one with this data...
			if (da[lpc] == insertTarget[j]["name"]) {
			    // found it!
			    //debug("found " + da[lpc] + " at " + lpc + "," + j);
			    insertTarget = insertTarget[j]["children"];
			    found = true;
			    break;
			}
		    }
		    if (!found) {
			// have to make a node:
			var tnode = {};
			tnode.data = [{"key":size_key}];
			tnode.children = [];
			tnode.id = da[lpc] + "-" + colorfield;
			tnode.name = da[lpc];

			//debug("creating for " + da[lpc]);

			// add the node to the parent
			insertTarget[insertTarget.length] = tnode;

			//debug("created " + da[lpc] + " at " + lpc + "," + (insertTarget.length - 1));

			// set the insertTarget for the next iteration
			insertTarget = tnode.children;
		    }
		}
		// Here's where we "fix" ca.{id,name} so they work.
		// Ugh, we have to compose the id of *all* parents in 
		// the chain, else there are problems!
		var multi_id = '';
		for (var lpc = 0; lpc < num_multi + 1; ++lpc) {
		    multi_id += da[lpc];
		    if ((lpc + 1) < (num_multi +  1)) {
			multi_id += '-';
		    }
		}
		//ca.id = da[1+num_multi-1] + "-" + colorfield;
		ca.id = multi_id;
		ca.name = da[1+num_multi-1];
		insertTarget[insertTarget.length] = ca;
	    }
	    else {
		objects[colorfield]["children"][ccount] = ca;
	    }
	    // don't set value because this is not a leaf.
	    objects[colorfield]["data"] = [{"key":size_key}];
	    ++cfcount;
	}
	++ccount;
    }

    /*
     * Make sure we note that the data is multilevel, if it is.
     */
    if (num_multi > 0) {
	for (var colorfield in objects) {
	    objects[colorfield]["num_multi"] = num_multi;
	    debug("num_multi = " + num_multi);
	}
    }

    /*
     * Set values for non-leaf nodes.
     */
    for (var colorfield in objects) {
	sumChildren(objects[colorfield]);
    }

    /*
    objstr = ""
    for (var o in objects) {
	objstr += " // " + o + " : " + objects[o]["children"].length;
    }

    $('rviz').innerHTML = "hdr = " + hdr.toString() + "<br/>" + 
	"total: " + size_total + "<br/>" + 
	"ccount: " + ccount + "<br/>" + 
	"debugstr: " + debugstr + "<br/>" +
	"objstr: " + objstr;
    */

    return { "objects":objects,"colorvalues":colorvals };
}

/*
 * Expensive, fix later.
 */
function sumChildren(tree) {
    if (tree.children.length == 0) {
	return tree.data[0].value;
    }

    tree.data[0].value = 0;
    for (var i = 0; i < tree.children.length; ++i) {
	tree.data[0].value += sumChildren(tree.children[i]);
    }
    //debug("sum of tree at node id " + tree.id + "(" + tree.name + ") = " + tree.data[0].value + " ; cl = " + tree.children.length);

    return tree.data[0].value;
}

var currentData = null;
var autoRefreshIntervalId = null;
var autoRefreshInterval = 0;

function setAutoRefresh(selectObj) {
    autoRefreshInterval = parseInt(selectObj.options[selectObj.selectedIndex].value);
    
    if (autoRefreshIntervalId) {
	clearInterval(autoRefreshIntervalId);
	autoRefreshIntervalId = null;
    }

    if (autoRefreshInterval) {
	autoRefreshIntervalId = setInterval("refresh(resource)",autoRefreshInterval);
    }
}
    

function changeColor(selectObj) {
    currentColor = selectObj.options[selectObj.selectedIndex].value;

    //debug("currentColor = " + currentColor);

    // reset color config so that the color gen doesn't blow up on us
    Config.Color.minValue = currentData["colorvalues"][currentColor]["min"];
    Config.Color.maxValue = currentData["colorvalues"][currentColor]["max"];
    tm.loadFromJSON(currentData["objects"][currentColor]);
}

function update(text,xml) {
    currentData = parseAndBuild(text);

    /* Need a header for multilevel tree data so users can navigate. */
    if (!(currentData["objects"][currentColor]["num_multi"] == undefined) 
	&& currentData["objects"][currentColor]["num_multi"] > 0) {
	Config.titleHeight = 18;
	debug("tH = 18");
    }

    if (!(currentData["colorvalues"] == undefined)) {
	var cfbstr = '<select id=colorprop onchange=changeColor(this)>';
	for (colorfield in currentData["objects"]) {
	    cfbstr += "<option value=" + colorfield;
	    if (colorfield == currentColor) {
		cfbstr += " selected";
	    }
	    cfbstr += ">" + colorfield + "</option>";
        }
	cfbstr += "</select>";
	$('colorfieldbox').innerHTML = cfbstr;

	// reset color config so that the color gen doesn't blow up on us
	Config.Color.minValue = currentData["colorvalues"][currentColor]["min"];
	Config.Color.maxValue = currentData["colorvalues"][currentColor]["max"];
	tm.loadFromJSON(currentData["objects"][currentColor]);
    }
    else {
	Config.Color.allow = false;
	$('colorfieldbox').innerHTML = '';
    }

    // restart updates, if they should be happening
    if (autoRefreshInterval && !autoRefreshIntervalId) {
        autoRefreshIntervalId = setInterval("refresh(resource)",autoRefreshInterval);
    }

    // set a date in the footer to stamp the data
    $('rvizfooter').innerHTML = '<center>Data last updated at' + 
	' <b>' + (new Date()).toLocaleString() + '</b></center>';
}

function refresh(rresource) {
    // if they preempt their own auto update rate, we adjust here and 
    // reset the auto refresh after the update
    if (autoRefreshIntervalId) {
        clearInterval(autoRefreshIntervalId);
	autoRefreshIntervalId = null;
    }

    var pargs = "uid=" + tbuid + "&auth=" + tbauth;
    pargs += "&getdata=1&resource=" + rresource;
    var req = new Request({ url:TBBASE + "/rusage_viz.php",method:'get',autoCancel:true,async:true });
    req.onSuccess = update;
    req.send(pargs);
}

function resizeVizForceRedraw() {
    resizeViz(true);
}

function resizeViz(redrawViz) {
    /*
     * Expand div to max size permitted by window.
     */
    var size = Window.getSize();
    var header = $('rvizheader');
    var rviz = $('rviz');
    var headerOffset = header.getSize().y;

    var newStyles = {
	'height': Math.floor(size.y - 175),
	'width' : Math.floor(size.x - 32)
    };
    rviz.setProperties(newStyles);
    rviz.setStyles(newStyles);
    //rviz.setStyles({'position':'absolute','top': headerOffset + 'px'});

    if (redrawViz) {
	tm.loadFromJSON(currentData["objects"][currentColor]);
    }
}

function init() {
    resizeViz(false);

    /*
     * Setup config object for JIT treemap.
     */
    Config.rootId = 'rviz';
    // if you change this, must add more to the mootools lib (need Tips)
    Config.tips = false;

    Config.Color.allow = true;
    // Set min value and max value for the second *dataset* object values.
    // Default's to -100 and 100.
    Config.Color.minValue = 1;
    Config.Color.maxValue = 50;
    //Set color range. Default's to reddish and greenish. It takes an array of three
    //integers as R, G and B values.
    Config.Color.minColorValue = [0, 235, 50];
    Config.Color.maxColorValue = [235, 0, 50];
    
    //Allow tips for treemap
    Config.tips = true;
    
    Config.titleHeight = 0;
	
    tm = new TM.Squarified();
    //We define a controller to the treemap instance.
    tm.controller = {
	//The interface method to be called, treemap element creation.
        onCreateElement: function(domElement, node) {

	    //debug("creating element " + node.id + "(" + node.name + ")("
	    //+ node.children.length + ")");

	    width = parseInt(domElement.style.width);
	    height = parseInt(domElement.style.height);
	    font_size = Math.ceil(Math.sqrt(width*height)/8);
	    data_font_size = Math.ceil(font_size * .75);
	    if (font_size < 8) {
		font_size = 8;
	    }
	    if (data_font_size < 8) {
		data_font_size = 8;
	    }
	    iHTML = '<table border="0" style="color:#ffffff" cellpadding="0" cellspacing="0" width="100%" height="100%"><tr><td style="font-size:' + font_size + 'px;text-align:left; vertical-align:top;">'+node["name"]+'</td></tr>';
	    //	    iHTML = '';
	    iHTML += '<tr><td style="font-size:' + data_font_size + 'px; text-align:right;vertical-align:bottom;">';
	    for (var i = 0; i < node["data"].length; ++i) {
		da = node["data"][i];
		iHTML += '<span style="margin-right:5px">';
		da_value = da["value"];
		if (!(da["label_value"] == undefined)) {
		    da_value = da["label_value"];
		}
		iHTML += ''+da_value+' '+da["key"];
		iHTML += '</span><br>';
	    }
	    iHTML += '</td></tr></table>';

	    domElement.innerHTML = iHTML;

	    //this.makeTip(domElement, node);
	},
	//Tooltip content is setted by setting the *title* of the element to be *tooltiped*.
	//Read the mootools docs for further understanding.
	makeTip: function(elem, json) {
	    var title = json.name;
	    var html = this.makeHTMLFromData(json.data);
	    //elem.set('title', title + '::' + html);
	},
	//Take each dataset object key and value and make an HTML from it.
	makeHTMLFromData: function(data) {
	    return '';
	}
    };

    refresh(resource);
}

addLoadFunction(init);
window.onresize = resizeVizForceRedraw;

</script>

<div id="errorbox"></div>
<div id="rvizheader" style="text-align:left;font-weight:normal;padding:8px;font-size:11px;font-family:Verdana,Arial,Helvetica,sans-serif">

<?php

echo "Resource views: ";
foreach ($resources as $r) {
    echo "<a href=?resource=$r>$r</a> ";
}

?>

&nbsp;
<input type=button id="refresh" value="Manual Refresh" onclick=refresh(resource)>
&nbsp;
Auto Refresh: 
<select id="autorefresh" onchange="setAutoRefresh(this)">
  <option value=0 selected>No Refresh</option>
  <option value=30000>30 seconds</option>
  <option value=60000>1 minute</option>
  <option value=120000>2 minutes</option>
  <option value=300000>5 minutes</option>
  <option value=600000>10 minutes</option>
  <option value=900000>15 minutes</option>
  <option value=1800000>30 minutes</option>
</select>
&nbsp;
Color property: <span id="colorfieldbox"></span>
</div>

<div style="width: 600px; height: 500px" id="rviz"></div>

<div id="rvizfooter" style="text-align:left;padding:8px;font-size:12px;font-family:Verdana,Arial,Helvetica,sans-serif"></div>

<?php

PAGEFOOTER();

?>
