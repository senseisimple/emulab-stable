<?php
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Show Experiment Information");

#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

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
$exp_eid = $eid;
$exp_pid = $pid;

#
# Check to make sure this is a valid PID/EID tuple.
#
if (! TBValidExperiment($exp_pid, $exp_eid)) {
  USERERROR("The experiment $exp_eid is not a valid experiment ".
            "in project $exp_pid.", 1);
}

#
# Verify Permission.
#
if (! TBExptAccessCheck($uid, $exp_pid, $exp_eid, $TB_EXPT_READINFO)) {
    USERERROR("You do not have permission to view experiment $exp_eid!", 1);
}

function WRITESIDEBARSUBBUTTON($text, $link) {
    echo "<!-- Table row for button $text -->
          <tr>
            <td valign=center align=left nowrap>
                <b>
         	 <a class=sidebarbutton href='$link'>$text</a>\n";
    #
    # XXX these blanks look bad in lynx, but add required
    #     spacing between menu and body
    #
    echo "       &nbsp;&nbsp;\n";

    echo "      </b>
            </td>
          </tr>\n";
}

echo "<table cellspacing=2 cellpadding=2 width='85%' border=0>\n";
echo "<tr>\n";
echo "   <td valign=top>\n";
echo "       <table cellspacing=2 cellpadding=2 border=0 width=200>\n";
echo "         <tr><td align=center><b>Experiment Options</b></td></tr>\n";
echo "         <tr></tr>\n";
WRITESIDEBARSUBBUTTON("View NS File and Node Assignment",
		      "shownsfile.php3?pid=$exp_pid&eid=$exp_eid");
WRITESIDEBARSUBBUTTON("Terminate this experiment",
		      "endexp.php3?pid=$exp_pid&eid=$exp_eid");

# Swap option.
$expstate = TBExptState($exp_pid, $exp_eid);
if ($expstate) {
    if (strcmp($expstate, $TB_EXPTSTATE_SWAPPED) == 0) {
	WRITESIDEBARSUBBUTTON("Swap this Experiment in",
		      "swapexp.php3?inout=in&pid=$exp_pid&eid=$exp_eid");
	WRITESIDEBARSUBBUTTON("Graphic Visualization of Topology",
		      "vistopology.php3?pid=$exp_pid&eid=$exp_eid");
    }
    elseif (strcmp($expstate, $TB_EXPTSTATE_ACTIVE) == 0) {
	WRITESIDEBARSUBBUTTON("Swap this Experiment out",
		      "swapexp.php3?inout=out&pid=$exp_pid&eid=$exp_eid");
	WRITESIDEBARSUBBUTTON("Graphic Visualization of Topology",
		      "vistopology.php3?pid=$exp_pid&eid=$exp_eid");
    }
}

#
# Admin folks get a swap request link to send email.
#
if (ISADMIN($uid)) {
    WRITESIDEBARSUBBUTTON("Send a swap/terminate request",
			  "request_swapexp.php3?&pid=$exp_pid&eid=$exp_eid");
}
echo "       </table>\n";
echo "   </td>\n";
echo "   <td valign=top align=left width='85%'>\n";

#
# Dump experiment record.
# 
SHOWEXP($exp_pid, $exp_eid);

echo "   </td>\n";
echo " </tr>\n";
echo "</table>\n";

#
# Dump the node information.
#
SHOWNODES($exp_pid, $exp_eid);

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
