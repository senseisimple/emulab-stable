<?php
#
# Beware empty spaces (cookies)!
#
# The point of this file is so that when people go to www.emulab.net 
# they will be redirected from http://www.emulab.net/index.html to
# https://www.emulab.net/start.php3, so that we can force certain traffic
# through the secure server instead of the plain server. $TBBASE sets the
# base pointer for the secure server. 
# 
require("defs.php3");

header("Location: " . "$TBBASE" . "index.php3");

?>

