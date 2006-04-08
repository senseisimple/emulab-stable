/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * Provides a controller for a Google maps like view that can be dragged around
 * by the mouse.  The code is mostly taken from:
 *
 *   http://markpasc.org/weblog/2005/05/28/landmarker_or_so_youd_like_to_make_an_ajax_map
 *
 * It works by moving an absolutely positioned inner div (the "substrate")
 * around in an outer div (the "viewport") as the user drags the mouse around.
 *
 * @todo This might actually work better by using just the substrate with a
 * "clip" style.
 *
 * @see The template_show.php for an example usage.
 *
 * Google maps is the best...
 * True dat!
 * Double True!
 */

DragView = {};

/**
 * Construct a drag view controller.
 *
 * @param viewport The element that acts as the viewport.
 * @param substrate The element that will be moved around inside the viewport.
 * Must be a direct descendent of the viewport.
 * @param bounds Optional parameter that specifies the width and height for the
 * content of the substrate.  If this is given, the viewport will always keep
 * the content atleast partially in view.
 */
DragView.Controller = function(viewport, substrate, bounds) {
    var self = this; // Reference to _this_ object from the event handlers.

    /* Add client side state for all drag views. */
    if (!("DragView" in ClientState)) {
	ClientState.DragView = {};
    }

    /* Initialize the state for this view. */
    if (!(substrate.id in ClientState.DragView)) {
	ClientState.DragView[substrate.id] = {};
	ClientState.DragView[substrate.id].left = "0px";
	ClientState.DragView[substrate.id].top = "0px";
    }
    
    /* Retrieve the saved state for this view. */
    substrate.style.left = ClientState.DragView[substrate.id].left;
    substrate.style.top = ClientState.DragView[substrate.id].top;

    this.minViewable = 50; // Number of pixels of content that must be viewable
    this.downLeft = 0;
    this.downTop = 0;
    this.downX = 0;
    this.downY = 0;
    this.isDragging = false;

    this.move = function(evt) {
	if (!evt) var evt = window.event;
	if (this != viewport && this != substrate) return true;

	if (!self.isDragging) {
	    /*
	     * Start the drag when the cursor is moved.  We change to the
	     * "dragging" class from "nodrag" so any child elements can adapt.
	     */
	    substrate.className = "dragsubstrate dragging";
	    /* Switch to a move cursor. */
	    this.style.cursor = "move";
	    self.isDragging = true;
	}

	var newLeft, newTop;

	/* Compute the new left and top coordinates, then */
	newLeft = self.downLeft + evt.screenX - self.downX;
	newTop = self.downTop + evt.screenY - self.downY;

	/* ... check them against the bounds, if there are any. */
	if (bounds) {
	    if (newLeft < (self.minViewable - bounds.width))
		newLeft = (self.minViewable - bounds.width);
	    if (newLeft > (viewport.clientWidth - self.minViewable))
		newLeft = (viewport.clientWidth - self.minViewable);

	    if (newTop < (self.minViewable - bounds.height))
		newTop = (self.minViewable - bounds.height);
	    if (newTop > (viewport.clientHeight - self.minViewable))
		newTop = (viewport.clientHeight - self.minViewable);
	}
	
	/*
	 * Update the substrate with the new coordinates.  NB: left/top in the
	 * style structure are strings with units, not just numbers.
	 */
	substrate.style.left = newLeft + "px";
	substrate.style.top = newTop + "px";
    };

    /**
     * Switch to/from "zoom" mode.  Currently, this is done by switch between
     * the "dvzoomin" and "dvzoomout" classes.
     *
     * @param doZoom True if the view should be in zoomed mode, false otherwise
     */
    this.zoom = function(doZoom) {
	if (doZoom)
	    viewport.className = "dragviewport dvzoomin";
	else
	    viewport.className = "dragviewport dvzoomout";
    };

    /**
     * Apply event handlers to the given element.
     *
     * @param element The element to apply the behaviours to.
     */
    this.behaviour = function(element) {
	element.ondragstart = function(evt) { return false; }; // XXX IE
	element.onmousedown = function(evt) {
	    if (!evt) var evt = window.event;
	    
	    if ((evt.which && evt.which == 3) ||
		(evt.button && evt.button == 2)) {
		return true;
	    }
	    
	    self.downLeft = parseInt(substrate.style.left);
	    self.downTop = parseInt(substrate.style.top);
	    /* Record where the mouse was clicked. */
	    self.downX = evt.screenX;
	    self.downY = evt.screenY;

	    this.onmousemove = self.move;

	    return true;
	};
	element.onmouseup = function(evt) {
	    this.style.cursor = "auto";
	    this.onmousemove = null;

	    substrate.className = "dragsubstrate nodrag";
	    /* Save state for future loads. */
	    ClientState.DragView[substrate.id].left = substrate.style.left;
	    ClientState.DragView[substrate.id].top = substrate.style.top;
	    self.isDragging = false;

	    return true;
	};
	element.onmouseout = function(evt) {
	    return element.onmouseup(evt);
	};
    };

    /* Apply our behaviours to the viewport. */
    this.behaviour(viewport);
};
