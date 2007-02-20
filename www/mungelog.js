/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006, 2007 University of Utah and the Flux Group.
 * All rights reserved.
 */
var LOG_STATE_LOADING = 1;
var LOG_STATE_LOADED = 2;

/* The experiment pid/eid used when getting the pnode list. */
var exp_pid = "";
var exp_eid = "";

var pnodes = new Array(); // List of pnode names in longest to shortest order.
var lastLength = 0; // The length of the download text at the last check.
var lastLine = ""; // The last line of the download text.

/*
 * The progress of the GetPNodes() call.
 *   0 - Not called yet.
 *   1 - Request sent, waiting for the reply.
 *   2 - Reply received.
 */
var getPNodeProgress = 0;

var nextState = LOG_STATE_LOADING; // The state of the log download.
var docTriesLeft = 5; // Tries before giving up on getting the document.

var lastError = -2;
var maxLineLength = 110;

/*
 * True for the first line of the log.  Used to counteract the hack that sends
 * 1024 spaces to flush the log.
 */
var firstLine = true;

/*
 * Start the interval that will read from the download iframe and update the
 * main body of the document.
 */
var upInterval = setInterval('ml_handleReadyState(nextState)', 1000);

/* Callback for the GetPNodes ajax function. */
function GetPNodes_cb(data) {
    pnodes = data;
    getPNodeProgress = 2;
    ml_handleReadyState(nextState);
}

/* Clear the various 'loading' indicators. */
function ml_loadFinished(done) {
    clearInterval(upInterval);

    if (done) {
        ClearLoadingIndicators("<center><b>Done!</b></center>");
    }
    else {
        ClearLoadingIndicators(" ");
    }

    nextState = LOG_STATE_LOADED;
}

function ml_getInnerText(el) {
	if (typeof el == "string") return el;
	if (typeof el == "undefined") { return ""; };
	if (el.innerText) return el.innerText;	//Not needed but it is faster
	var str = "";
	
	var cs = el.childNodes;
	var l = cs.length;
	for (var i = 0; i < l; i++) {
		switch (cs[i].nodeType) {
			case 1: //ELEMENT_NODE
				str += ml_getInnerText(cs[i]);
				break;
			case 3:	//TEXT_NODE
				str += cs[i].nodeValue;
				break;
		}
	}
	return str;
}

/*
 * @param ifr The iframe to retrieve the text from.
 * @return The text in the given iframe.  If the text is surrounded by '<pre>'
 * tags (mozilla, IE), they will be stripped.
 */
function ml_getBodyText(ifr) {
    var retval = null;

    try {
	var oDoc;
	
	if (ifr.document && is_safari) {
	    // Safari
	    oDoc = ifr.document;
        }
	else {
	    oDoc = (ifr.contentWindow || ifr.contentDocument);
	    if (oDoc.document) {
		oDoc = oDoc.document;
	    }
	}
	for (lpc = 0; lpc < oDoc.childNodes.length; lpc++) {
	    text = ml_getInnerText(oDoc.childNodes[lpc]);
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

/* @return The innerHeight of the window. */
function ml_getInnerHeight() {
    var retval;
    var win = getObjbyName('outputframe').contentWindow;
    var doc = getObjbyName('outputframe').contentWindow.document;

    if (win.innerHeight) // all except Explorer
      retval = win.innerHeight;
    else if (doc.documentElement && doc.documentElement.clientHeight)
      // Explorer 6 Strict Mode
	retval = doc.documentElement.clientHeight;
    else if (doc.body) // other Explorers
      retval = doc.body.clientHeight;

    return retval;
}

/* @return The scrollTop of the window. */
function ml_getScrollTop() {
    var retval;
    var win = getObjbyName('outputframe').contentWindow;
    var doc = getObjbyName('outputframe').contentWindow.document;

    if (win.pageYOffset) // all except Explorer
      retval = win.pageYOffset;
    else if (document.documentElement && document.documentElement.scrollTop) // Explorer 6 Strict
      retval = document.documentElement.scrollTop;
    else if (document.body) // all other Explorers
      retval = document.body.scrollTop;
    
    return retval;
}

/* @return The height of the document. */
function ml_getScrollHeight() {
    var retval;
    var win = getObjbyName('outputframe').contentWindow;
    var doc = getObjbyName('outputframe').contentWindow.document;
    var test1 = doc.body.scrollHeight;
    var test2 = doc.body.offsetHeight;

    if (test1 > test2) // all but Explorer Mac
      retval = doc.body.scrollHeight;
    else // Explorer Mac;
    //would also work in Explorer 6 Strict, Mozilla and Safari
      retval = doc.body.offsetHeight;

    return retval;
}

/*
 * Main function used to copy data from the download iframe to the body of the
 * document.
 *
 * @param state The state of the download.
 */
function ml_handleReadyState(state) {
    var Iframe = getObjbyName('outputframe');
    var idoc   = IframeDocument('outputframe');
    var oa     = idoc.getElementById('outputarea');
    var dl     = getObjbyName('downloader');

    if (docTriesLeft < 0) {
        /* Already decided we were broken; just ignore */
        return;
    }

    if ((rt = ml_getBodyText(dl)) == null) {
	/* 
	 * Browsers that do not support DOMs for text/plain files or are a
	 * little slow in getting it setup will end up here.  If we fail to
	 * get the document after a couple of tries, we bail out and just
	 * make the iframe visible.
	 */
	docTriesLeft -= 1;
	if (docTriesLeft < 0) {
	    /* Give up, turn off the spinner */
	    ml_loadFinished(0);

            /* Hide the outputarea */
            HideFrame("outputframe");

            /* Make the downloader frame visible. */
	    var winheight = GetMaxHeight('downloader');

	    dl.style.border = "2px solid";
	    dl.height       = winheight;
	    dl.width        = "100%";
	    dl.scrolling    = "auto";
	    dl.frameBorder  = "1";
	}
	return;
    }

    if (state == LOG_STATE_LOADED) {
	ml_loadFinished(1);
    }
    
    if (state == LOG_STATE_LOADING || state == LOG_STATE_LOADED) {
	/*
	 * Get any new data and append it to the last line from the previous
	 * iteration (in case the line was only partially downloaded).
	 */
	var newData = lastLine + rt.substring(lastLength);

	/* Look for assigns "signal" that the pnode list is available. */
	/* XXX Turned off since it is slow. */
	if (0 && newData.indexOf('Mapped to physical reality!') != -1) {
	    if (getPNodeProgress == 1) {
		/* Still waiting for the reply. */
		return;
	    }
	    else if (getPNodeProgress == 0) {
		/* Send the request. */
		getPNodeProgress = 1;
		x_GetPNodes(exp_pid, exp_eid, GetPNodes_cb);
		
		return;
	    }
	}

	lastLength = rt.length;
	lastLine = "";

	/*
	 * Record the size of the document before modifying it so we can see
	 * if we can automatically do the scroll.
	 */
	var ih = ml_getInnerHeight();
	var y = ml_getScrollTop();
	var h = ml_getScrollHeight();
	
	var lines = newData.split("\n");
	var plain = "";
	/* Iterate over whatever lines we get. */
	for (i = 0; i < lines.length - 1; i++) {
	    var line = lines[i];
	    var matches = new Array();
	    var lengths = new Array();
	    var hasError = 0;

	    if (!firstLine && line.length > maxLineLength) {
		var lastwhite = line.lastIndexOf(" ", maxLineLength);

		if (lastwhite <= 0)
		  lastwhite = maxLineLength;
		nextLine = line.substring(lastwhite);
		line = line.substring(0, lastwhite) + " \\";
		lines.splice(i + 1, 0, nextLine)
	    }
	    
	    /* Check for errors. */
	    if (line.indexOf('***') != -1 ||
		(lastError == i - 1) && line.indexOf('   ') == 0) {
		if (plain != "") {
		    tn = idoc.createTextNode(plain);
		    oa.appendChild(tn);
		}
		plain = "";
		hasError = 1;
		lastError = i;
	    }
	    else {
		lastError = -2;
	    }

	    /*
	     * Look for pnodes to turn into links.  The list is sorted by the
	     * length of the name so longer names will match before shorter
	     * (e.g. 'pc201' before 'pc2').
	     */
	    for (lpc = 0; lpc < pnodes.length; lpc++) {
		var pnode = pnodes[lpc];
		var index = 0;
		
		while ((index = line.indexOf(pnode, index)) != -1) {
		    /* Make sure a longer name was not already matched. */
		    if (!(index in lengths)) {
			matches.push(index);
			lengths[index] = pnode;
		    }
		    index += pnode.length;
		}
	    }
	    
	    /* Sort the indexes so we do them in order. */
	    matches.sort(function(a, b) { return a - b; });
	    
	    var lastIndex = 0;
	    
	    if (matches.length > 0) {
		for (var j in matches) {
		    if (j == '______array') // js idiocy
		      continue;
		    
		    index = matches[j];
		    pnode = lengths[index];
		    
		    plain += line.substring(lastIndex, index);
		    tn = idoc.createTextNode(plain);
		    if (hasError) {
			fn = idoc.createElement("font");
			fn.setAttribute("color", "red");
			fn.appendChild(tn);
			oa.appendChild(fn);
		    }
		    else {
			oa.appendChild(tn);
		    }
		    plain = "";
		    
		    /* Create the link. */
		    var linktext = line.substring(index, index + pnode.length);
		    var nlink = idoc.createElement("A");
		    nlink.setAttribute('href',
				       'shownode.php3?node_id=' + pnode);
		    nlink.setAttribute('target', '_parent');
		    tn = idoc.createTextNode(linktext);
		    nlink.appendChild(tn);
		    oa.appendChild(nlink);
		    
		    lastIndex = index + pnode.length;
		}
	    }
	    if (hasError) {
		/* It is an error line, turn it red. */
		tn = idoc.createTextNode(line.substring(lastIndex));
		fn = idoc.createElement("font");
		fn.setAttribute("color", "red");
		fn.appendChild(tn);
		oa.appendChild(fn);
		lastIndex = line.length;
	    }

	    if (!firstLine || line.search(/[^ \t\n]/) != -1) {
		plain += line.substring(lastIndex) + "\n";
	    }
	    firstLine = false;
	}
	lastLine = lines[lines.length - 1];

	if (lastError != -2) {
	    lastError = -1;
	}
	
	if (state == LOG_STATE_LOADED)
	  plain += lastLine;

	tn = idoc.createTextNode(plain);
	oa.appendChild(tn);

	var nh = ml_getScrollHeight();

	/* See if we should scroll the window down. */
	if ((h - (y + ih)) < (y == 0 ? 200 : 10)) {
	    idoc.documentElement.scrollTop = nh;
	    idoc.body.scrollTop = nh;
	}
    }
}
