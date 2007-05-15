/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006, 2007 University of Utah and the Flux Group.
 * All rights reserved.
 */
/*
 * Some utility stuff.
 */

var user_agent = navigator.userAgent.toLowerCase();
var is_safari   = false;

if (user_agent.indexOf("safari") != -1) {
    is_safari = true;
}

/* Clear the various 'loading' indicators. */
function ClearLoadingIndicators(done_msg)
{
    var busyimg = getObjbyName('busy');
    var loadingspan = getObjbyName('loading');

    loadingspan.innerHTML = done_msg;
    busyimg.style.display = "none";
    busyimg.src = "1px.gif";
}

function ClearBusyIndicators(done_msg)
{
	ClearLoadingIndicators(done_msg);
}

/* Replace the current page */
function PageReplace(URL)
{
    window.location.replace(URL);
}

function IframeDocument(id) 
{
    var oIframe = getObjbyName(id);
    var oDoc    = (oIframe.contentWindow || oIframe.contentDocument);

    if (oIframe.document && is_safari) {
        // Safari
	return oIframe.document;
    }
    
    if (oDoc.document) {
	return oDoc.document;
    }
    return oDoc;
}

function GraphChange_cb(html) {
    grapharea = getObjbyName('grapharea');
    if (grapharea) {
        grapharea.innerHTML = html;
    }
}

function GraphChange(which) {
    var arg0 = "all";
    var arg1 = "all";
    
    var srcvnode = getObjbyName('trace_srcvnode');
    if (srcvnode) {
	arg0 = srcvnode.options[srcvnode.selectedIndex].value;
    }
    var dstvnode = getObjbyName('trace_dstvnode');
    if (dstvnode) {
	arg1 = dstvnode.options[dstvnode.selectedIndex].value;
    }

    x_GraphShow(which, arg0, arg1, GraphChange_cb);
    return false;
}

function GetMaxHeight(id) {
    var Iframe    = getObjbyName(id);
    var winheight = 0;
    var yoff      = 0;

    // This tells us the total height of the browser window.
    if (window.innerHeight) // all except Explorer
	winheight = window.innerHeight;
    else if (document.documentElement &&
	     document.documentElement.clientHeight)
	// Explorer 6 Strict Mode
	winheight = document.documentElement.clientHeight;
    else if (document.body)
	// other Explorers
	winheight = document.body.clientHeight;

    // Now get the Y offset of the outputframe.
    yoff = Iframe.offsetTop;

    if (winheight != 0) {
	// Now calculate how much room is left and make the iframe
	// big enough to use most of the rest of the window.
	if (yoff != 0)
	    winheight = winheight - (yoff + 175);
	else
	    winheight = winheight * 0.7;
    }
    return winheight;
}

function SetupOutputArea(id) {
    var Iframe    = getObjbyName(id);
    var IframeDoc = IframeDocument(id);
    var winheight = GetMaxHeight(id);

    IframeDoc.open();
    IframeDoc.write('<html><head><base href=$BASEPATH/></head><body><pre id=outputarea></pre></body></html>');
    IframeDoc.close();

    Iframe.style.border = "2px solid";
    Iframe.style.width  = "100%";
    Iframe.height       = winheight;
    Iframe.width        = "100%"; 
    Iframe.scrolling    = "auto";
    Iframe.frameBorder  = "1";
}

function HideFrame(id) {
    var Iframe    = getObjbyName(id);
    var IframeDoc = IframeDocument(id);

    Iframe.style.border = "0px none";
    Iframe.frameBorder  = 0;
    Iframe.height       = 0;
    Iframe.width        = 0;
    Iframe.scrolling    = "no";
}

function ShowDownLoader(id) {
    var Iframe    = getObjbyName(id);
    var IframeDoc = IframeDocument(id);

    var winheight = 0;
    var yoff = 0;

    // This tells us the total height of the browser window.
    if (window.innerHeight) // all except Explorer
	winheight = window.innerHeight;
    else if (document.documentElement &&
	     document.documentElement.clientHeight)
	// Explorer 6 Strict Mode
	winheight = document.documentElement.clientHeight;
    else if (document.body)
	// other Explorers
	winheight = document.body.clientHeight;

    // Now get the Y offset of the outputframe.
    yoff = Iframe.offsetTop;

    if (winheight != 0) {
	// Now calculate how much room is left and make the iframe
	// big enough to use most of the rest of the window.
	if (yoff != 0)
	    winheight = winheight - (yoff + 175);
	else
	    winheight = winheight * 0.7;
    }

    Iframe.style.border = "2px solid";
    Iframe.height       = winheight;
    Iframe.width        = "100%";
    Iframe.scrolling    = "auto";
    Iframe.frameBorder  = "1";
}

/* @return The innerHeight of the window. */
function getInnerHeight(id) {
    var retval;
    var win = getObjbyName(id).contentWindow;
    var doc = getObjbyName(id).contentWindow.document;

    if (win.innerHeight)
	// all except Explorer
	retval = win.innerHeight;
    else if (doc.documentElement && doc.documentElement.clientHeight)
	// Explorer 6 Strict Mode
	retval = doc.documentElement.clientHeight;
    else if (doc.body)
	// other Explorers
	retval = doc.body.clientHeight;

    return retval;
}

/* @return The scrollTop of the window. */
function getScrollTop(id) {
    var retval;
    var win = getObjbyName(id).contentWindow;
    var doc = getObjbyName(id).contentWindow.document;

    if (win.pageYOffset)
	// all except Explorer
	retval = win.pageYOffset;
    else if (document.documentElement && document.documentElement.scrollTop)
	// Explorer 6 Strict
	retval = document.documentElement.scrollTop;
    else if (document.body)
	// all other Explorers
	retval = document.body.scrollTop;
    
    return retval;
}

/* @return The height of the document. */
function getScrollHeight(id) {
    var retval;
    var win = getObjbyName(id).contentWindow;
    var doc = getObjbyName(id).contentWindow.document;
    var test1 = doc.body.scrollHeight;
    var test2 = doc.body.offsetHeight;

    if (test1 > test2)
	// all but Explorer Mac
	retval = doc.body.scrollHeight;
    else
	// Explorer Mac;
	// would also work in Explorer 6 Strict, Mozilla and Safari
	retval = doc.body.offsetHeight;

    return retval;
}

/* Write the text to the output area, keeping the cursor at the bottom. */
var lastLength = 0;  // The length of the download text at the last update
var lastLine   = ""; // The last line of the download text.
var firstLine  = 1;  // Flag.

function WriteOutputText(id, stuff) {
    var Iframe = getObjbyName(id);
    var idoc   = IframeDocument(id);
    var output = idoc.getElementById('outputarea');

    /*
     * Append new stuff to the last line from the previous call in case
     * the previous line was only partially downloaded.
     */
    var newData = lastLine + stuff.substring(lastLength);
    var lines   = newData.split("\n");
    var newText = "";

    // Globals
    lastLength = stuff.length;
    lastLine   = "";

    /*
     * Record the size of the document before modifying it so we can see
     * if we can automatically do the scroll.
     */
    var ih = getInnerHeight(id);
    var y  = getScrollTop(id);
    var h  = getScrollHeight(id);
	
    /* Iterate over lines */
    for (i = 0; i < lines.length - 1; i++) {
	var line = lines[i];

	if (firstLine) {
	    if (line.search(/^[ \ ]*$/) >= 0) {
		continue;
	    }
	    firstLine = 0;
	}
	newText += line + "\n";
    }
    // For next call
    lastLine = lines[lines.length - 1];

    // Add the new text to the DOM.
    textNode = idoc.createTextNode(newText);
    output.appendChild(textNode);

    /* See if we should scroll the window down. */
    var nh = getScrollHeight(id);

    if ((h - (y + ih)) < (y == 0 ? 200 : 10)) {
	idoc.documentElement.scrollTop = nh;
	idoc.body.scrollTop = nh;
    }
}

function GetInnerText(el) {
    if (typeof el == "string")
	return el;
    if (typeof el == "undefined")
	return "";
    // Not needed but it is faster    
    if (el.innerText)
	return el.innerText;

    var str = "";
    var cs  = el.childNodes;
    var l   = cs.length;
    
    for (var i = 0; i < l; i++) {
	switch (cs[i].nodeType) {
	case 1: //ELEMENT_NODE
	    str += GetInnerText(cs[i]);
	    break;
	case 3:	//TEXT_NODE
	    str += cs[i].nodeValue;
	    break;
	}
    }
    return str;
}

/*
 * @return The text in the given iframe.  If the text is surrounded by '<pre>'
 * tags (mozilla, IE), they will be stripped.
 */
function GetInputText(id) {
    var ifr    = getObjbyName(id);
    var retval = null;

    try {
	var oDoc = (ifr.contentWindow || ifr.contentDocument);
	if (oDoc.document) {
	    oDoc = oDoc.document;
	}
	for (lpc = 0; lpc < oDoc.childNodes.length; lpc++) {
	    text = GetInnerText(oDoc.childNodes[lpc]);
	    if (text != "") {
		if (retval == null)
		    retval = "";
		retval += text;
	    }
	}
	if (retval.indexOf("<pre>") != -1 || retval.indexOf("<PRE>") != -1) {
	    retval = retval.substring(5, retval.length - 6);
	}
    }
    catch (error) {
    }

    return retval;
}

/*
 * Process, start and stop output.
 */
var OutputInterval;
var InputID;
var OutputID;

function LookforOutput() {
    var stuff;
    
    if ((stuff = GetInputText(InputID)) != null) {
	WriteOutputText(OutputID, stuff);
    }
}

function StartOutput(input_id, output_id) {
    OutputID  = output_id;
    InputID   = input_id;
    firstLine = 1;
    OutputInterval = setInterval('LookforOutput()', 1000);
}

function StopOutput() {
    clearInterval(OutputInterval);
    LookforOutput();
}

/*
 * Cross browser support for finding objects by id.
 */
function getObjbyName(name) {
    if (document.getElementById) {
	return document.getElementById(name);
    }
    else if (document.all) {
	return document.all[name];
    }
    else if (document.layers) {
	return getObjNN4(document,name);
    }
    return null;
}
function getObjNN4(obj,name) {
    var x = document.layers;
    var foundLayer;
    
    for (var i=0; i < x.length; i++) {
	if (x[i].id == name)
	    foundLayer = x[i];
	else if (x[i].layers.length)
	    var tmp = getObjNN4(x[i],name);
	
	if (tmp) foundLayer = tmp;
    }
    return foundLayer;
}
