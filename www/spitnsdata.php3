<?php
include("defs.php3");

#
# Spit back an NS file to the user. It comes in as an urlencoded argument
# and we send it back as plain text so that user can write it out. This
# is in support of the graphical NS topology editor that fires off an
# experiment directly.
#

#
# See if nsdata was provided. 
#
if (!isset($nsdata) ||
    strcmp($nsdata, "") == 0) {
    PAGEERROR("No NS file provided!");
}

header("Content-Type: text/plain");
echo "$nsdata";

?>
