<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#
#
# Default definitions, selected via configure.
# See doc/setup.txt in the root of the source tree for
# explanations of some of these variables.
# 
$WWWHOST        = "www.wide.emulab.net";
$WWW            = "$WWWHOST";
$TBBASE         = "https://$WWW";
$TBDOCBASE      = "http://$WWW";
$TBWWW          = "<$TBBASE/>";
$TBAUTHDOMAIN   = "wide.emulab.net";
$TBSECURECOOKIES= 1;
$TBCOOKIESUFFIX = "";
$TBMAINSITE     = 0;

#
# Title Page stuff.
#
$TITLECOLOR     = "#E04050";
$BANNERCOLOR    = "#ABABE0";
$THISHOMEBASE   = "Wide.Emulab.Net";
$THISPROJECT    = "The Wide-Area Network Testbed";

$TBMAILADDR_OPS		= "widetestbed-ops@flux.cs.utah.edu";
$TBMAIL_OPS		= "Testbed Ops <$TBMAILADDR_OPS>";
$TBMAILADDR_WWW		= "widetestbed-www@flux.cs.utah.edu";
$TBMAIL_WWW		= "Testbed WWW <$TBMAILADDR_WWW>";
$TBMAILADDR_APPROVAL	= "widetestbed-approval@flux.cs.utah.edu";
$TBMAIL_APPROVAL	= "Testbed Approval <$TBMAILADDR_APPROVAL>";
$TBMAILADDR_LOGS	= "widetestbed-logs@flux.cs.utah.edu";
$TBMAIL_LOGS		= "Testbed Logs <$TBMAILADDR_LOGS>";
$TBMAILADDR_AUDIT	= "widetestbed-audit@flux.cs.utah.edu";
$TBMAIL_AUDIT		= "Testbed Audit <$TBMAILADDR_AUDIT>";
?>
