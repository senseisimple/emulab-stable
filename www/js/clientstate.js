/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * Hash used to store client side state that should be available to a group of
 * pages.  So, if you want to save some state on the client side, add your
 * object(s) to this hash and then read/write to them as usual.
 *
 * XXX This state will end up getting transferred to the server on requests,
 * which kinda sucks...
 */
ClientState = {};

ClientStateFunctions = {
    /**
     * Save the client-side state stored in the ClientState object into a
     * cookie.
     *
     * @todo Add a parameter so state can be separated into different groups so
     * they don't intermingle.
     */
    saveState: function() {
	cookieLib.setCookie("cs", ClientState, 1);
    },
    /**
     * Load the client-side state from the cookie into ClientState.
     */
    loadState: function() {
	var tmp = cookieLib.getCookie("cs");
	
	if (tmp != null)
	    ClientState = tmp;
    }
};
