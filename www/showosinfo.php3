<?php
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("OSID Information");

#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Verify form arguments.
# 
if (!isset($osid) ||
    strcmp($osid, "") == 0) {
    USERERROR("You must provide an OSID.", 1);
}

if (! TBValidOSID($osid)) {
    USERERROR("The OSID `$osid' is not a valid OSID.", 1);
}

#
# Verify permission.
#
if (!TBOSIDAccessCheck($uid, $osid, $TB_OSID_READINFO)) {
    USERERROR("You do not have permission to access OSID $osid!", 1);
}

#
# Dump os_info record.
# 
SHOWOSINFO($osid);

# Terminate option.
if (TBOSIDAccessCheck($uid, $osid, $TB_OSID_DESTROY)) {
    echo "<p><center>
           Do you want to remove this OSID?
           <A href='deleteosid.php3?osid=$osid'>Yes</a>
          </center>\n";
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
