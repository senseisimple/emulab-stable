<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#
#
# Default definitions. Selected via configure.
# 
$WWWHOST        = "www.mini.emulab.net";
$WWW            = "$WWWHOST";
$TBBASE         = "https://$WWW";
$TBDOCBASE      = "http://$WWW";
$TBWWW          = "<$TBBASE/>";
$TBAUTHDOMAIN   = ".emulab.net";
$TBSECURECOOKIES= 1;
$TBCOOKIESUFFIX = "-mini";

#
# Title Page stuff.
#
$TITLECOLOR     = "#E04050";
$BANNERCOLOR    = "#ABABE0";
$THISHOMEBASE   = "Mini.Emulab.Net";
$THISPROJECT    = "The Mini Network Testbed";

$TBMAILADDR_OPS		= "minitestbed-ops@flux.cs.utah.edu";
$TBMAIL_OPS		= "Testbed Ops <$TBMAILADDR_OPS>";
$TBMAILADDR_WWW		= "minitestbed-www@flux.cs.utah.edu";
$TBMAIL_WWW		= "Testbed WWW <$TBMAILADDR_WWW>";
$TBMAILADDR_APPROVAL	= "minitestbed-approval@flux.cs.utah.edu";
$TBMAIL_APPROVAL	= "Testbed Approval <$TBMAILADDR_APPROVAL>";
$TBMAILADDR_LOGS	= "minitestbed-logs@flux.cs.utah.edu";
$TBMAIL_LOGS		= "Testbed Logs <$TBMAILADDR_LOGS>";
$TBMAILADDR_AUDIT	= "minitestbed-audit@flux.cs.utah.edu";
$TBMAIL_AUDIT		= "Testbed Audit <$TBMAILADDR_AUDIT>";
?>
