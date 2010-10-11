<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2010 University of Utah and the Flux Group.
# All rights reserved.
#
# This is just a convenience; redirect to the real login page.
#
require("defs.php3");

header("Location: https://$WWWHOST/login.php3?adminmode=1");

?>
