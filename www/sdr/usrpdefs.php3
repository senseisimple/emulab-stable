<?php
#
# Definitions for GNU radio pages.
#
chdir("..");
require("defs.php3");
chdir("sdr");

$USRP_MENU		    = array();
$USRP_MENU['title']	    = "USRP Menu";
$USRP_MENU['Home']	    = "index.php3";
$USRP_MENU['FAQ']	    = "docwrapper.php3?docname=faq.html&title=".
                               "'Frequently Asked Questions'";
$USRP_MENU['Recent News']   = "docwrapper.php3?docname=news.html&title=".
                               "'Recent News'";
$USRP_MENU['USRP Users']    = "docwrapper.php3?docname=users.html&title=".
                               "'USRP Users'";
$USRP_MENU['Public Wiki']   = "https://${USERNODE}/twiki/bin/view/SDR";

$USRP_MENUDEFS = array('hide_sidebar'     => 1,
		       'hide_banner'      => 1,
		       'hide_versioninfo' => 1,
		       'menu'		  => $USRP_MENU,
		       );
?>
