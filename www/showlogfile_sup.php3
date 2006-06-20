<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
require_once("Sajax.php");
sajax_init();
sajax_export("GetPNodes", "GetExpState");

# If this call is to client request function, then turn off interactive mode.
# All errors will go to above function and get reported back through the
# Sajax interface.
if (sajax_client_request()) {
    $session_interactive = 0;
}

function CHECKPAGEARGS($pid, $eid) {
    global $uid, $TB_EXPT_READINFO;
    
    #
    # Verify page arguments.
    # 
    if (!isset($pid) ||
	strcmp($pid, "") == 0) {
	USERERROR("You must provide a Project ID.", 1);
    }
    if (!isset($eid) ||
	strcmp($eid, "") == 0) {
	USERERROR("You must provide an Experiment ID.", 1);
    }
    if (!TBvalid_pid($pid)) {
	PAGEARGERROR("Invalid project ID.");
    }
    if (!TBvalid_eid($eid)) {
	PAGEARGERROR("Invalid experiment ID.");
    }

    #
    # Check to make sure this is a valid PID/EID tuple.
    #
    if (! TBValidExperiment($pid, $eid)) {
	USERERROR("The experiment $pid/$eid is not a valid experiment!", 1);
    }
    #
    # Verify permission.
    #
    if (! TBExptAccessCheck($uid, $pid, $eid, $TB_EXPT_READINFO)) {
	USERERROR("You do not have permission to view the log for $pid/$eid!", 1);
    }
}

function GetPNodes($pid, $eid) {
    CHECKPAGEARGS($pid, $eid);
    
    $retval = array();

    $query_result = DBQueryFatal(
	"select r.node_id from reserved as r ".
	"where r.eid='$eid' and r.pid='$pid' order by LENGTH(node_id) desc");

    while ($row = mysql_fetch_array($query_result)) {
      $retval[] = $row[node_id];
    }

    return $retval;
}

function GetExpState($pid, $eid)
{
    CHECKPAGEARGS($pid, $eid);
    
    $expstate = TBExptState($pid, $eid);

    return $expstate;
}

function STARTWATCHER($pid, $eid)
{
    echo "<script type='text/javascript' language='javascript'
                  src='showexp.js'></script>\n";

    $currentstate = TBExptState($pid, $eid);
    
    echo "<script type='text/javascript' language='javascript'>\n";
    sajax_show_javascript();
    echo "StartStateChangeWatch('$pid', '$eid', '$currentstate');\n";
    echo "</script>\n";
}

function STARTLOG($pid, $eid)
{
    global $BASEPATH;

    STARTWATCHER($pid, $eid);
    
    echo "<script type='text/javascript' language='javascript'>\n";
    echo "function SetupOutputArea() {
              var Iframe = document.getElementById('outputframe');
	      var IframeDoc = IframeDocument('outputframe');
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

	      IframeDoc.open();
	      IframeDoc.write('<html><head><base href=$BASEPATH/></head><body><pre id=outputarea></pre></body></html>');
 	      IframeDoc.close();

              if (winheight != 0)
                  // Now calculate how much room is left and make the iframe
                  // big enough to use most of the rest of the window.
                  if (yoff != 0)
                      winheight = winheight - (yoff + 175);
                  else
                      winheight = winheight * 0.7;

	          Iframe.height = winheight;
          }
          </script>\n";

    echo "<center>\n";
    echo "<img id='busy' src='busy.gif'>
                   <span id='loading'> Working ...</span>";
    echo "</center>\n";
    echo "<br>\n";
    
    echo "<div><iframe id='outputframe' src='busy.gif' ".
	"width=100% height=600 scrolling=auto border=4></iframe></div>\n";
    
    echo "<script type='text/javascript' language='javascript' src='json.js'>
          </script>".
	 "<script type='text/javascript' language='javascript'
                  src='mungelog.js'>
          </script>\n";
    echo "<script type='text/javascript' language='javascript'>\n";

    echo "SetupOutputArea();\n"; 

    echo "exp_pid = \"$pid\";\n";
    echo "exp_eid = \"$eid\";\n";
    echo "</script>
         <iframe id='downloader' name='downloader' width=0 height=0
                 src='spewlogfile.php3?pid=$pid&eid=$eid'
                 onload='ml_handleReadyState(LOG_STATE_LOADED);'
                 border=0 style='width:0px; height:0px; border: 0px'>
         </iframe>\n";
}

# See if this request is to one of the above functions. Does not return
# if it is. Otherwise return and continue on.
sajax_handle_client_request();

#
# We return to the including script ...
#
