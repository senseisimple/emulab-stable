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

DelayBox = {};

DelayBox.Controller = function() {
    var self = this;
    
    this.timeout = null;
    this.sched = function() {
	if (self.timeout != null)
	    clearTimeout(self.timeout);

	self.timeout = setTimeout(self.timeoutHandler, 500);
    };
    this.timeoutHandler = function() {
	self.timeout = null;
	self.handler();
    };

    this.behaviour = function(element) {
	element.onkeyup = self.sched;
    };
};
