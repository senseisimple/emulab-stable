<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#

#
# An example definitions, selected via the WWWDEFS variable in the
# main defs file.
# 

#
# Domain the testbed resides in
#
$TBAUTHDOMAIN   = "example.emulab.net";

#
# Fully-quaified hostname of the webserver
#
$WWWHOST        = "www.emulab.net";

#
# Title Page stuff - customize to suit
#
$TITLECOLOR     = "#E04050";
$BANNERCOLOR    = "#ABABE0";
$THISHOMEBASE   = "Example.Emulab.Net";
$THISPROJECT    = "The Example Network Testbed";

#
# You shouldn't need to change anything below this point.
#
$WWW            = "$WWWHOST";
$TBBASE         = "https://$WWW";
$TBDOCBASE      = "http://$WWW";
$TBWWW          = "<$TBBASE/>";
$TBSECURECOOKIES= 1;
$TBCOOKIESUFFIX = "";
$TBMAINSITE     = 0;

?>
