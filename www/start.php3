<?php
#
# Beware empty spaces (cookies)!
#
# The point of this file is so that when people go to www.emulab.net 
# they will be redirected from http://www.emulab.net/index.html to
# https://www.emulab.net/start.php3, so that we can force certain traffic
# through the secure server instead of the plain server. 
# 
require("defs.php3");

#
# We want to redirect to emulab, not paper. This needs to be fixed!
# 
if (isset($SSL_PROTOCOL)) {
    $LOC = "$TBBASE/index.php3";
}
else {
    $LOC = "$TBDOCBASE/index.php3";
}

header("Location: $LOC");

?>
