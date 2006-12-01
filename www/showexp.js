/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */
/*
 * State change handling code for show experiment page.
 */

// This is the window we popup to tell the user about the state change.
var myWindow;

// This is the timer to kill the window after a little while.
var WindowTimer;

// This is the timer for the state change watcher below.
var StateChangeTimer;

// Number of minutes (in ms of course) before we kill the popup.
var POPUPTIME = (5 * 60000);

// Experiment being watched.
var pid;
var eid;
var laststate;

// The statechange function.

function StateChangePopup(pid, eid, state)
{
    var newWinHeight = 250;
    var newWinWidth  = 500;
    var newWinTop    = (screen.height-newWinHeight)/2;
    var newWinLeft   = (screen.width-newWinWidth)/2;
    
    var winProps = eval('"resizable=no,menubar=no,toolbar=no,scrollbars=no,top=" +newWinTop+ ",left=" +newWinLeft+ ",width=" +newWinWidth+ ",height=" +newWinHeight');

    var myPage = '/statechange.php?pid=' + pid + '&eid=' + eid +
	'&state=' + state;

    myWindow = window.open(myPage, "statechange", winProps);
    WindowTimer  = window.setTimeout("StateChangePopupTimeout()", POPUPTIME);
}

// Timeout function

function StateChangePopupTimeout()
{
    myWindow.close();
    myWindow = null;
}

// Manual kill
function StateChangePopupKill()
{
    clearTimeout(WindowTimer);
    WindowTimer = null;
    
    myWindow.close();
    myWindow = null;
}

// Okay, setup a timer to call the Sajax function to get the state.
function StartStateChangeWatch(exp_pid, exp_eid, currentstate)
{
    pid = exp_pid;
    eid = exp_eid;
    laststate = currentstate;
    StateChangeTimer  = window.setTimeout("StateChangeWatch()", 30000);
}

// Callback for for below.
function GetExpState_cb(newstate)
{
    /*
     * The only state change we notify is going to active.
     */
    if (newstate != laststate) {
	td = getObjbyName("exp_state");
	if (td) {
	    td.innerHTML = newstate;
	}
	if (newstate == "active") {
	    StateChangePopup(pid, eid, newstate);
	}
    }
    laststate = newstate;
    StateChangeTimer = window.setTimeout("StateChangeWatch()", 30000);
}

// Wake up and ask Emulab for the current experiment state.
function StateChangeWatch()
{
    x_GetExpState(pid, eid, GetExpState_cb);
}


