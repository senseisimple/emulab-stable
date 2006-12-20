<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

if (!$CHATSUPPORT) {
    header("Location: index.php3");
    return;
}

# No Pageheader since we spit out a redirection below.
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();

$password  = $this_user->mailman_password();
$url       = "${OPSJETIURL}?user=$uid&password=$password";

header("Location: ${url}");
?>
