
Template.GUID = null;
Template.vers = null;
Template.vers2node = {};

if (!("Template" in ClientState)) {
    debug("no template state!!!");
    ClientState.Template = {
	SHOW_ALL: false
    };
}

Template.Node = function(dbs) {
    Template.vers2node[dbs.vers] = this;
    this.vers = dbs.vers;
    this.parent_vers = dbs.parent_vers;
    this.hidden = dbs.hidden;
    this.parent = null;
    this.model = dbs;
    this.next = null;
    this.searchHighlight = false;
    this.paths = [];
    this.children = [];
};

Template.DataSource = function() {
    this.rootForTree = function() {
	var retval;

	if (1 in Template.vers2node)
	    retval = Template.vers2node[1];
	else
	    retval = null;

	return retval;
    };

    this.bodyForHead = function(th) {
	return "";
    };

    this.initNode = function(node) {
	if (node.model.searchHighlight) {
	    node.userdata.childNodes[0].style.borderLeftColor = "orange";
	}
    };

    this.pathsForTag = function(tag) {
	return tag.paths;
    };

    this.bodyForTag = function(tag) {
	return "<div class='tname'>" + tag.model.tid + "</div>";
    };

    this.bodyForPopup = function(tag) {
	var retval = "Version: " + tag.vers;

	retval += "<p class='popupcontent'>";
	if ("metadata" in tag.model) {
	    var keys = $H(tag.model.metadata).keys();

	    for (var lpc = 0; lpc < keys.length; lpc++) {
		var name = keys[lpc];

		value = tag.model.metadata[name];
		retval += "<span class='metadata'>"
		    + name
		    + ": "
		    + value
		    + "</span>";
	    }
	}
	if ("parameters" in tag.model) {
	    var keys = $H(tag.model.parameters).keys();

	    retval += "</p><h3>Parameters:</h3><p class='popupcontent'>";

	    for (var lpc = 0; lpc < keys.length; lpc++) {
		var name = keys[lpc];

		value = tag.model.parameters[name];
		retval += "<span class='parameter'>"
		    + name
		    + ": "
		    + value
		    + "</span>";
	    }
	}
	retval += "</p>";

	return retval;
    };

    this.tagArray = function(th) {
	var retval = new Array();
	var curr = th;

	while (curr != null) {
	    if (ClientState.Template.SHOW_ALL || curr.hidden == 0) {
		retval.push(curr);
	    }
	    curr = curr.next;
	}

	return retval;
    };

    this.branchesForTag = function(th, tag) {
	var retval = new Array();

	for (var lpc = 0; lpc < tag.children.length; lpc++) {
	    var child = tag.children[lpc];

	    if (ClientState.Template.SHOW_ALL || child.hidden == 0) {
		retval.push(child);
	    }
	}

	return retval;
    };
};

Template.DBtoNode = function(dbstate) {
    Template.vers2node = {};
    for (var lpc = 0; lpc < dbstate.length; lpc++) {
	var next = new Template.Node(dbstate[lpc]);
	
	if (next.parent_vers != null) {
	    var par = Template.vers2node[next.parent_vers];
	    
	    next.parent = par;
	    if (par.next == null) {
		par.next = next;
	    }
	    else {
		    par.children.push(next);
	    }
	}
    }
};

Template.tracePath = function(curr, path) {
    while (curr != null) {
	curr.paths.push(path);
	curr = curr.parent;
    }
};

Template.updateSearch = function(dbstate) {
    for (var vers in Template.vers2node) {
	if (vers == '______array') // js idiocy
	    continue;

	Template.vers2node[vers].searchHighlight = false;
    }
    for (var lpc = 0; lpc < dbstate.length; lpc++) {
	var vers = dbstate[lpc];

	if (vers in Template.vers2node) {
	    Template.vers2node[vers].searchHighlight = true;
	}
    }
};

Template.Delegate = function() {
    this.selectionChanged = function(ttv, oldSelection, newSelection) {
	if (newSelection != null) {
	    window.location = "template_show.php?"
		+ "guid=" + Template.GUID
		+ "&version=" + newSelection.model.vers;
	}
    };
};
