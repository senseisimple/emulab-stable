/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2009 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Simple funciton to handle multiple onload/onunload events
 */
var LOADFUNCTIONS = [];
var UNLOADFUNCTIONS = [];
function addLoadFunction(func) {
    LOADFUNCTIONS.push(func);
}

function addUnloadFunction(func) {
    UNLOADFUNCTIONS.push(func);
}

/*
 * Just loop through our arrays calling all of the appropriate functions
 */
function callAllLoadFunctions() {
    for (var i = 0; i < LOADFUNCTIONS.length; i++) {
        LOADFUNCTIONS[i].call();
    }
}

function callAllUnloadFunctions() {
    for (var i = 0; i < UNLOADFUNCTIONS.length; i++) {
        UNLOADFUNCTIONS[i].call();
    }
}

/*
 * Set the onload and onunload functions to the ones we just made
 */
window.onload = callAllLoadFunctions;
window.onunload = callAllUnloadFunctions;

