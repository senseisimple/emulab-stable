<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
# Emulate register_globals off on the fly.
#
$emulating_on = 0;

function EmulateRegisterGlobals()
{
    global $emulating_on;
    
    if ($emulating_on)
	return;

    $emulating_on++;

    # 
    # Start out slow ...
    #
    $superglobals = array($_GET, $_POST, $_COOKIE);

    //
    // Known PHP Reserved globals and superglobals:
    //    
    $knownglobals = array(
			  '_ENV',       'HTTP_ENV_VARS',
			  '_GET',       'HTTP_GET_VARS',
			  '_POST',	'HTTP_POST_VARS',
			  '_COOKIE',    'HTTP_COOKIE_VARS',
			  '_FILES',     'HTTP_FILES_VARS',
			  '_SERVER',    'HTTP_SERVER_VARS',
			  '_SESSION',   'HTTP_SESSION_VARS',
			  '_REQUEST',	'emulating_on'
			 );

    foreach ($superglobals as $superglobal) {
	foreach ($superglobal as $global => $void) {
	    if (!in_array($global, $knownglobals)) {
		unset($GLOBALS[$global]);
	    }
	}
    }

    error_reporting(E_ALL);
}

EmulateRegisterGlobals();
