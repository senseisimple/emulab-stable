<?php
#
# EMULAB-COPYRIGHT
#   Copyright (c) 2009 University of Utah and the Flux Group.
#   All rights reserved.
#

include("../defs.php3");
include("usermap.php");

$optargs = OptionalPageArguments("fullscreen",   PAGEARG_BOOLEAN);

if (isset($fullscreen)) {
    draw_usermap($USERMAP_TYPE_FULLSCREEN);
} else {
    draw_usermap($USERMAP_TYPE_NORMAL);
}
