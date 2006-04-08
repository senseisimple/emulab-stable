/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * A "time tree" is a widget that displays a timeline that starts at the left
 * side of the screen and grows to the right.  Along this timeline there can be
 * branches where new timelines are started.
 *
 * @todo Get rid of any hardcoded constants for the size/look of the tree
 * icons.
 */

var TimeTree = {};

/*
 * The width and height of the icons used in the tree.
 *
 * XXX These should be a per-tree field.
 */
TimeTree.ICO_WIDTH = 63;
TimeTree.ICO_HEIGHT = 35;

function ttnEmptyHandler(ttn) {
}

/**
 * Construct the head of a branch.
 *
 * @param model The model object this view is bound to.
 * @param rowBody The HTML contents of the row.
 */
TimeTree.Head = function(model, rowBody) {
    this.model = model;
    this.layoutData = {};

    /* Create a row layer that will contain all points on the timeline. */
    this.node = document.createElement("div");
    this.node.innerHTML = rowBody;

    /* Pointer to the head of the list of TimeTree.Node's */
    this.head = null;
};

/**
 * Construct a node on a branch.
 *
 * @param model The model object this view is bound to.
 * @param branches The initial array of branches off this node.
 * @param popupBody The HTML contents of the mouseover popup.
 */
TimeTree.Node = function(model, branches, userBody, popupBody) {
    this.model = model;
    this.branches = branches;
    this.layoutData = {};
    this.next = null;

    this.stopper = function(event) {
	Event.stop(event);
    };

    /* The root layer for this node.  Also used to display the path circles. */
    this.node = document.createElement("div");
    this.node.setAttribute("class", "base");

    /* Layer that contains the tree image itself. */
    this.treenode = document.createElement("div");
    this.treenode.setAttribute("class", "auto");
    this.node.appendChild(this.treenode);

    /* Layer for the mouse over highlight. */
    this.hoverlight = document.createElement("div");
    this.hoverlight.setAttribute("class", "hoverlight");
    this.node.appendChild(this.hoverlight);

    /* Element to contain any user data. */
    this.userdata = document.createElement("div");
    this.userdata.setAttribute("class", "userdata");
    this.userdata.innerHTML = userBody;
    this.userdata.onclick = this.stopper;
    this.userdata.onmousemove = this.stopper;
    this.node.appendChild(this.userdata);

    /* Create the popup that contains a short description of the node. */
    var popup = document.createElement("div");
    popup.setAttribute("class", "popup");
    popup.innerHTML = popupBody;
    popup.onclick = this.stopper;
    popup.onmousemove = this.stopper;
    this.node.appendChild(popup);

    var self = this;
    this.onclick = function(event) {
	self.view.setSelection(self);

	return false;
    };

    this.node.onclick = this.onclick;
};

/**
 * Construct the object used to layout a tree.
 *
 * @param parent The parent HTMLElement where the tree elements should be
 * added.
 */
TimeTree.Layout = function(parent) {
    this.parent = parent;
    this.topDirection = { "up" : 1.0, "down" : -1.0 };
    this.baseLeft = null;
    this.baseTop = null;

    /**
     * @param ld The layout data for a node.
     * @return The total height of the node, including sub-timelines.
     */
    this.totalHeight = function(ld) {
	return ld.height.up + ld.height.down + TimeTree.ICO_HEIGHT;
    };

    /**
     * @param ld The layout data for a head node.
     * @return The orientation to use for the next sub-timeline.
     */
    this.nextOrientation = function(ld) {
	var retval = ld.nextOrientation;
	
	if (ld.nextOrientation == "up")
	    ld.nextOrientation = "down";
	else
	    ld.nextOrientation = "up";
	
	return retval;
    };
    
    /**
     * Layout a TimeTree.Head object.  The layout is pretty simple, we just
     * figure out how much height is needed above and below this row for any
     * branches.  We do not try to be smart and put branches on the same row
     * when they would fit, every branch gets its own row and the ones further
     * to the right are closer to the root row.
     *
     * @param root The timeline to layout.
     */
    this.doLayout = function(root) {
	if (root == null)
	    return;

	var curr = root.head;

	/* Initialize the layout data for this node. */
	root.layoutData["width"] = 0;
	root.layoutData["height"] = { "up" : 0, "down" : 0 };
	root.layoutData["nextOrientation"] = "up";
	while (curr != null) {
	    for (var lpc = 0; lpc < curr.branches.length; lpc++) {
		var tn = curr.branches[lpc];
		var ld = tn.layoutData;
		
		/*
		 * Record whether or not the node has a neighbor above or below
		 * so we know to draw a connector between them.
		 */
		if (lpc == 0) {
		    curr.layoutData["n" + root.layoutData.nextOrientation] = 1;
		}
		if ((lpc + 2) < curr.branches.length) {
		    curr.branches[lpc + 2].head.
			layoutData["n" + root.layoutData.nextOrientation] = 1;
		}

		/* Pick whether the node will be placed above or below. */
		ld["orientation"] = this.nextOrientation(root.layoutData);

		/* Record more neighbor info. */
		if (lpc == 0 && curr.branches.length >= (lpc + 2)) {
		    curr.layoutData["n" + root.layoutData.nextOrientation] = 1;
		}
		curr.branches[lpc].head.
		    layoutData["n" + root.layoutData.nextOrientation] = 1;

		/* Layout the sub-timeline. */
		this.doLayout(tn);

		/* Increment the height we require. */
		root.layoutData["height"][ld.orientation] +=
		    this.totalHeight(ld);
	    }
	    root.layoutData.width += TimeTree.ICO_WIDTH;
	    curr = curr.next;
	}
    };

    /**
     * Create a filler line.
     *
     * @param left The left coordinate for the line.
     * @param top The top coordinate for the line.
     * @return The new DIV element.
     */
    this.createFiller = function(left, top) {
	var retval = document.createElement("div");

	retval.setAttribute("class", "filler");
	retval.style.backgroundImage = "url('timetree/empty1010.png')";
	retval.style.top = top + "px";
	retval.style.left = left + "px";
	return retval;
    };

    /**
     * Update the coordinates for the tree elements and add them to the DOM.
     *
     * @param left The offset from the left edge in pixels.
     * @param top The offset from the top edge in pixels.
     * @param root The timeline to add to the DOM.
     */
    this.updateDOM = function(left, top, root) {
	if (root == null)
	    return;

	var curr = root.head;
	var tops = { "up" : top, "down" : 0 };

	if (this.baseLeft == null) {
	    this.baseLeft = left;
	    this.baseTop = top;
	}

	/* Push down, past any branches above this row. */
	top += root.layoutData.height.up;
	/*
	 * Branches below are added at the bottom and built up towards this
	 * main row.
	 */
	tops.down = top + TimeTree.ICO_HEIGHT + root.layoutData.height.down;

	/* Alternate timerow classes. */
	root.node.className = "timerow"
	    + (((top - this.baseTop) % (TimeTree.ICO_HEIGHT * 2)) / TimeTree.ICO_HEIGHT)
	    + " " + root.node.className;

	/* 
	 * The top coordinate for this row, all nodes inside will be relative
	 * to this node.
	 */
	root.node.style.top = top + "px";

	/* First, we add the row for the timeline, then */
	this.parent.appendChild(root.node);

	/* ... we add the individual nodes. */
	while (curr != null) {
	    /* Figure out which image to use based on the neighbor info. */
	    bgimg = "conn";
	    bgimg += ("nup" in curr.layoutData) ? "1" : "0";
	    bgimg += "1";
	    bgimg += ("ndown" in curr.layoutData) ? "1" : "0";
	    bgimg += (curr != root.head) ? "1" : "0";
	    bgimg += ".png";
	    curr.treenode.style.backgroundImage =
		"url(timetree/" + bgimg + ")";

	    /*
	     * Position the node relative to the left side of the row.  We
	     * don't have to set the y position since the row is at the correct
	     * position.
	     */
	    curr.node.style.left = left + "px";
	    root.node.appendChild(curr.node);

	    /* Add the sub-timelines. */
	    for (var lpc = 0; lpc < curr.branches.length; lpc++) {
		var tn = curr.branches[lpc];
		var ld = tn.layoutData;
		var gap;

		if (ld.orientation == "down") {
		    var ntop = tops.down - ld.height.down;

		    /* Add fill lines for branches below, */
		    while (tops.down > ntop) {
			tops.down -= TimeTree.ICO_HEIGHT;
			if ((lpc - 2) >= 0) {
			    root.node.appendChild(this.createFiller(
				left, tops.down - top));
			}
		    }
		    /* ... skip this row, and */
		    tops.down -= TimeTree.ICO_HEIGHT;
		    ntop -= ld.height.up + TimeTree.ICO_HEIGHT;
		    /* ... add fill lines for upper branches. */
		    while (tops.down > ntop) {
			tops.down -= TimeTree.ICO_HEIGHT;
			root.node.appendChild(this.createFiller(
				left, tops.down - top));
		    }
		}

		this.updateDOM(left, tops[ld.orientation], tn);

		if (ld.orientation == "up") {
		    var ntop = tops.up + ld.height.up;

		    /* Add fill lines for upper branches, */
		    while (tops.up < ntop) {
			if ((lpc - 2) >= 0) {
			    root.node.appendChild(this.createFiller(
				left, -tops.up));
			}
			tops.up += TimeTree.ICO_HEIGHT;
		    }
		    /* ... skip this row, and */
		    tops.up += TimeTree.ICO_HEIGHT;
		    ntop += ld.height.down + TimeTree.ICO_HEIGHT;
		    /* ... add fill lines for lower branches. */
		    while (tops.up < ntop) {
			root.node.appendChild(this.createFiller(
				left, -tops.up));
			tops.up += TimeTree.ICO_HEIGHT;
		    }
		}
	    }
	    if (curr.branches.length > 0) {
		var ftop = tops.up;
		
		/*
		 * Add any remaining fill lines caused by branches that are
		 * further to the right.
		 */
		while (ftop < root.layoutData.height.up) {
		    root.node.appendChild(this.createFiller(left, -ftop));
		    ftop += TimeTree.ICO_HEIGHT;
		}

		ftop = tops.down - TimeTree.ICO_HEIGHT;
		while (ftop > top) {
		    root.node.appendChild(this.createFiller(left, ftop - top));
		    ftop -= TimeTree.ICO_HEIGHT;
		}
	    }
	    left += TimeTree.ICO_WIDTH;
	    curr = curr.next;
	}
    };
};

/**
 * Constructor for the main object in a time tree view.
 *
 * @param parent The parent DOM element where the tree node elements should be
 * added.
 * @param dataSource The source of the content for the tree.
 */
TimeTree.View = function(parent, dataSource) {
    this.tl = new TimeTree.Layout(parent);
    this.ds = dataSource;
    this.delegate = new TimeTree.DebugDelegate();
    this.rootView = null;
    this.selection = null;
    this.bounds = { width: 0, height: 0 };

    /**
     * Construct the time tree internal nodes for a branch head by asking the
     * data source for the list of tags, branches, and element bodies.
     *
     * @param th The model branch head to add views for.
     * @return The new TimeTree.Head object.
     */
    this.addViews = function(th) {
	var lastNode = null;
	var retval;

	if (th == null)
	    return null;

	retval = new TimeTree.Head(th, this.ds.bodyForHead(th));
	retval.view = this;
	if ("initHead" in this.ds)
	    this.ds.initHead(retval);

	var ta = this.ds.tagArray(th);
	for (var lpc = 0; lpc < ta.length; lpc++) {
	    var branches = this.ds.branchesForTag(th, ta[lpc]);
	    var bviews = new Array();

	    for (var lpc2 = 0; lpc2 < branches.length; lpc2++) {
		bviews.push(this.addViews(branches[lpc2]));
	    }

	    var ttn = new TimeTree.Node(ta[lpc],
					bviews,
					this.ds.bodyForTag(ta[lpc]),
					this.ds.bodyForPopup(ta[lpc]));
	    ttn.view = this;
	    if ("initNode" in this.ds)
		this.ds.initNode(ttn);
	    if ("pathsForTag" in this.ds) {
		if (this.ds.pathsForTag(ta[lpc]).length > 0) {
		    ttn.node.style.backgroundImage =
			"url('timetree/path.png')";
		}
	    }

	    if (lastNode == null)
		retval.head = ttn;
	    else
		lastNode.next = ttn;
	    lastNode = ttn;
	}

	return retval;
    };

    /**
     * Reload content from the data source and relayout the tree.  Use this
     * when the content has changed.
     */
    this.reloadData = function() {
	this.rootView = this.addViews(this.ds.rootForTree());
	this.tl.doLayout(this.rootView);
	if (this.rootView != null) {
	    this.bounds.width = this.rootView.layoutData.width;
	    this.bounds.height = this.tl.totalHeight(this.rootView.layoutData);
	}
	else {
	    this.bounds.width = 0;
	    this.bounds.height = 0;
	}
    };

    this.updateDOM = function(left, top) {
	parent.innerHTML = "";
	this.tl.updateDOM(left, top, this.rootView);
    };

    /**
     * Internal callback triggered when a tree node has been clicked.
     */
    this.setSelection = function(ttn) {
	var os = this.selection;

	if (this.selection != null) {
	    this.selection.node.className =
		this.selection.node.className.replace(/ selected/, "");
	}
	this.selection = ttn;
	if (ttn != null) {
	    this.selection.node.className += " selected";
	    if ("selectionChanged" in this.delegate)
		this.delegate.selectionChanged(this, os, ttn);
	}
    };

    this.reloadData();
};

/**
 * Basic data source class for a time tree view.
 */
TimeTree.DataSource = function() {
    /**
     * @return The root model object for the tree.
     */
    this.rootForTree = function() {
	return null;
    };
    /**
     * @param th The model branch head.
     * @return The body for the TimeTree.Head element.
     */
    this.bodyForHead = function(th) {
	return "";
    };
    /**
     * @param tn The model object the popup is for.
     * @return The body for the popup that appears when the user mouses over
     * the tree node containing this model.
     */
    this.bodyForPopup = function(tn) {
	return "";
    };
    /**
     * @param th The model branch head.
     * @return An array of tags attached to this branch head.
     */
    this.tagArray = function(th) {
	return [];
    };
    /**
     * @param th The model branch head.
     * @param tag The model object that may or may not have branches.
     * @return Any branches hanging off this tag.
     */
    this.branchesForTag = function(th, tag) {
	return [];
    };
};

/**
 * "Debug" delegate for a time tree view.
 */
TimeTree.DebugDelegate = function() {
    this.selectionChanged = function(view, oldSelection, newSelection) {
	debug("selectionChanged("
	      + view
	      + ", "
	      + oldSelection
	      + ", "
	      + newSelection
	      + ")");
    };
};
