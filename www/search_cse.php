<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2008 University of Utah and the Flux Group.
# All rights reserved.
#
require("defs.php3");

#
# Verify page arguments.
#
$optargs = OptionalPageArguments("submit",      PAGEARG_STRING,
				 "query",       PAGEARG_STRING);

if (!isset($query)) {
    $query = "";
}

header("Location: http://www.google.com/cse?cx=007876920815929749329:hb-sxxwdr8y&ie=UTF-8&q=$query&sa=\"Search+Documentation\"");

?>

