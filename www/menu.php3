<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#

$login_status     = CHECKLOGIN_NOTLOGGEDIN;
$login_uid        = 0;
$drewheader       = 0;

#
# This has to be set so we can spit out http or https paths properly!
# Thats because browsers do not like a mix of secure and nonsecure.
# 
$BASEPATH	  = "";

#
# WRITESIDEBARBUTTON(text, link): Write a button on the sidebar menu.
# We do not currently try to match the current selection so that its
# link looks different. Not sure its really necessary.
#
function WRITESIDEBARBUTTON($text, $base, $link ) {
    $link = "$base/$link";
    echo "    <tr><td class=\"menuopt\"><a href=\"$link\">$text</a></td></tr>\n";
}

# same as above with "new" gif next to it.
function WRITESIDEBARBUTTON_NEW($text, $base, $link ) {
    $link = "$base/$link";
    echo "    <tr><td class=\"menuopt\"><a href=\"$link\">$text</a>&nbsp;<img src=\"/new.gif\" /></td></tr>\n";
}

function WRITESIDEBARBUTTON_ABS($text, $base, $link ) {
    $link = "$link";
    echo "    <tr><td class=\"menuopt\"><a href=\"$link\">$text</a></td></tr>\n";
}

# same as above, but uses a slightly different style sheet so there
# is more padding below the last button.
# The devil is, indeed, in the details.
function WRITESIDEBARLASTBUTTON($text, $base, $link) {
    $link = "$base/$link";
    echo "    <tr><td class=\"menuoptb\"><a href=\"$link\">$text</a></td></tr>\n";
}

# writes a message to the sidebar, without clickability.
function WRITESIDEBARNOTICE($text) {
    echo "    <tr><td class=\"menuopt\"><b>$text</b></td></tr>\n";
}

#
# WRITESIDEBAR(): Write the menu. The actual menu options the user
# sees depends on the login status and the DB status.
#
function WRITESIDEBAR() {
    global $login_status, $login_uid;
    global $TBBASE, $TBDOCBASE, $BASEPATH;
    global $THISHOMEBASE;

    #
    # The document base cannot be a mix of secure and nonsecure.
    #
    
    # create the main menu table, which also happens to reside in a form
    # (for search.)
?>
<FORM method=get ACTION="/cgi-bin/webglimpse/usr/testbed/webglimpse">
  <table class="menu" width=220 cellpadding="0" cellspacing="0">
    <tr><td class="menuheader"><b>Information</b></td></tr>
<?php
    WRITESIDEBARBUTTON("Home", $TBDOCBASE, "index.php3");
    WRITESIDEBARBUTTON_ABS("Kentucky Emulab", $TBDOCBASE,
			   "http://www.uky.emulab.net");
    
    WRITESIDEBARBUTTON_NEW("Tutorial at SIGCOMM",
		       $TBDOCBASE, "sc2002tut.php3");

    WRITESIDEBARBUTTON_NEW("News (August&nbsp;15)", $TBDOCBASE,
		       "docwrapper.php3?docname=news.html");
    WRITESIDEBARBUTTON("Documentation", $TBDOCBASE, "doc.php3");
    WRITESIDEBARBUTTON_NEW("Papers", $TBDOCBASE, "pubs.php3");
    WRITESIDEBARBUTTON("People", $TBDOCBASE, "people.php3");
    WRITESIDEBARBUTTON("The Gallery", $TBDOCBASE, "gallery/gallery.php3");
    WRITESIDEBARBUTTON("Projects on Emulab", $TBDOCBASE,
		       "projectlist.php3");
    WRITESIDEBARLASTBUTTON("Sponsors", $TBDOCBASE,
		       "docwrapper.php3?docname=sponsors.html");

    # create the search bit, then the second table for the Web Interface.
?>
    <tr><td class="menuoptst"><b>Search Documentation:</b></td></tr>
    <tr><td class="menuopts"><input name=query />
      <input type=submit style="font-size:10px;" value="Go" /></td></tr>
      <tr><td class="menuoptsb" style="font-size:12px;" >[
      <a href="<?php echo "$TBDOCBASE/search.php3"; ?>">Advanced Search</a> ]</td></tr>
    </td></tr>  
  </table>
</form>
<br>
<table class="menu" width=220 cellpadding="0" cellspacing="0">
    <tr><td class="menuheader"><b>Interaction</b></td></tr>
<?php

    if ($login_status & CHECKLOGIN_LOGGEDIN) {
         $freepcs = TBFreePCs();
	 WRITESIDEBARNOTICE( "($freepcs Free PCs.)" );
    }

    #
    # Basically, we want to let admin people continue to use
    # the web interface even when nologins set. But, we want to make
    # it clear its disabled.
    # 
    if (NOLOGINS()) {
        WRITESIDEBARBUTTON("<font color=red> ".
			   "Web Interface Temporarily Unavailable</font>",
			   $TBDOCBASE, "nologins.php3");

        if (!$login_uid || !ISADMIN($login_uid)) {	
	    WRITESIDEBARNOTICE("Please Try Again Later");
        }
    }

    if ($login_status & (CHECKLOGIN_LOGGEDIN|CHECKLOGIN_MAYBEVALID)) {
	if ($login_status & CHECKLOGIN_ACTIVE) {
	    if ($login_status & CHECKLOGIN_PSWDEXPIRED) {
		WRITESIDEBARBUTTON("Change Your Password",
				   $TBBASE, "moduserinfo.php3");
	    }
	    else {
		WRITESIDEBARBUTTON("My Emulab",
				   $TBBASE,
				   "showuser.php3?target_uid=$login_uid");
	    
		if (ISADMIN($login_uid)) {
		    WRITESIDEBARBUTTON("New Project Approval",
				       $TBBASE, "approveproject_list.php3");
		}
		if ($login_status & CHECKLOGIN_TRUSTED) {
                    # Only project/group leaders can do these options
		    WRITESIDEBARBUTTON("New User Approval",
				       $TBBASE, "approveuser_form.php3");
		}
		
                #
                # Since a user can be a member of more than one project,
                # display this option, and let the form decide if the user is
                # allowed to do this.
                #
		WRITESIDEBARBUTTON("Project List",
				   $TBBASE, "showproject_list.php3");
	    
		if (ISADMIN($login_uid)) {
		    WRITESIDEBARBUTTON("User List",
				       $TBBASE, "showuser_list.php3");
		}
	    
		WRITESIDEBARBUTTON("Experiment List",
				   $TBBASE, "showexp_list.php3");
		WRITESIDEBARBUTTON("Begin an Experiment",
				   $TBBASE, "beginexp.php3");
		WRITESIDEBARBUTTON("OSIDs and ImageIDs",
				   $TBBASE, "showosid_list.php3");
		WRITESIDEBARBUTTON("Update User Information",
				   $TBBASE, "moduserinfo.php3");
		WRITESIDEBARBUTTON("Node Reservation Status",
				   $TBBASE, "nodecontrol_list.php3");
		WRITESIDEBARBUTTON("Node Up/Down Status",
				   $TBDOCBASE, "updown.php3");
		
		if ($login_status & CHECKLOGIN_CVSWEB) {
		    WRITESIDEBARBUTTON("CVS Repository",
				       $TBBASE, "cvsweb/cvsweb.php3");
		}
	    }
	}
	elseif ($login_status & (CHECKLOGIN_UNVERIFIED|CHECKLOGIN_NEWUSER)) {
	    WRITESIDEBARBUTTON("New User Verification",
			       $TBBASE, "verifyusr_form.php3");
	    WRITESIDEBARBUTTON("Update User Information",
			       $TBBASE, "moduserinfo.php3");
	}
	elseif ($login_status & (CHECKLOGIN_UNAPPROVED)) {
	    WRITESIDEBARBUTTON("Update User Information",
			       $TBBASE, "moduserinfo.php3");
	}
    }

    #
    # Standard options for anyone.
    #
    if (! NOLOGINS()) {
	WRITESIDEBARBUTTON("Start Project", $TBBASE, "newproject.php3");
	WRITESIDEBARLASTBUTTON("Join Project",  $TBBASE, "joinproject.php3");
    }

    #
    # Cons up a nice message.
    # 
    switch ($login_status & CHECKLOGIN_STATUSMASK) {
    case CHECKLOGIN_LOGGEDIN:
	$login_message = "&quot;$login_uid&quot; Logged In.";
	    
	if ($login_status & CHECKLOGIN_PSWDEXPIRED)
	    $login_message = "$login_message<br>(Password Expired!)";
	elseif ($login_status & CHECKLOGIN_UNAPPROVED)
	    $login_message = "$login_message<br>(Unapproved!)";
	break;
    case CHECKLOGIN_TIMEDOUT:
	$login_message = "Login Timed Out";
	break;
    default:
	$login_message = 0;
	break;
    }

    if ($login_message) {
?>
    <tr>
      <td class="menufooter"><center><b><?php echo "$login_message"; ?></b></center></td>
    </tr>
<?php
    }

    #
    # Now the login/logout box. Remember, already inside a table.
    # We want the links to the login/logout pages to always be https,
    # but the images path depends on whether the page was loaded as
    # http or https, since we do not want to mix them, since they
    # cause warnings.
    # 
    if ($login_status & (CHECKLOGIN_LOGGEDIN|CHECKLOGIN_MAYBEVALID)) {

?>
    <tr>
      <td class="menufooter" align=center valign=center> <?php
echo "                <a href=\"$TBBASE/logout.php3?uid=$login_uid\">
	           <img alt=\"logoff\" border=0
                        src=\"$BASEPATH/logoff.gif\"></a>\n"
?>
      </td>
    </tr>
<?php

#	echo "<tr>
#               <td align=center height=50 valign=center>
#                <a href=\"$TBBASE/logout.php3?uid=$login_uid\">
#	           <img alt=\"logout\" border=0
#                        src=\"$BASEPATH/logoff.gif\"></a>
#               </td>
#              </tr>\n";
    }
    elseif (!NOLOGINS()) {
?>
    <tr>
      <td class="menufooter" align=center valign=center> <?php
echo "                <a href=\"$TBBASE/login.php3\">
	           <img alt=\"logon\" border=0
                        src=\"$BASEPATH/logon.gif\"></a>\n"
?>
      </td>
    </tr>
<?php

#	echo "<tr>
#               <td align=center height=50 valign=center>

#               </td>
#              </tr>\n";
    }


    #
    # MOTD. Set this with the webcontrol script.
    #
    $query_result =
	DBQueryFatal("SELECT message FROM loginmessage");
    
    if (mysql_num_rows($query_result)) {
    	$row = mysql_fetch_row($query_result);
	$message = $row[0];

	# XXX: make this red and big (like the old one)?
	WRITESIDEBARNOTICE($message);    
    }
    echo "</table>\n";
}

#
# Spit out a vanilla page header.
#
function PAGEHEADER($title) {
    global $login_status, $login_uid, $TBBASE, $TBDOCBASE, $THISHOMEBASE;
    global $BASEPATH, $SSL_PROTOCOL, $drewheader;
    global $TBMAINSITE;

    $drewheader = 1;

    #
    # Determine the proper basepath, which depends on whether the page
    # was loaded as http or https. This lets us be consistent in the URLs
    # we spit back, so that users do not get those pesky warnings. These
    # warnings are generated when a page *loads* (say, images, style files),
    # a mix of http and https. Links can be mixed, and in fact when there
    # is no login active, we want to spit back http for the documentation,
    # but https for the start/join pages.
    #
    if (isset($SSL_PROTOCOL)) {
	$BASEPATH = $TBBASE;
    }
    else {
	$BASEPATH = $TBDOCBASE;
    }

    #
    # Figure out who is logged in, if anyone.
    # 
    if (($known_uid = GETUID()) != FALSE) {
        #
        # Check to make sure the UID is logged in (not timed out).
        #
        $login_status = CHECKLOGIN($known_uid);
	if ($login_status & (CHECKLOGIN_LOGGEDIN|CHECKLOGIN_MAYBEVALID)) {
	    $login_uid = $known_uid;
	}
    }

    #
    # Check for NOLOGINS. 
    # We want to allow admin types to continue using the web interface,
    # and logout anyone else that is currently logged in!
    #
    if (NOLOGINS() && $login_uid && !ISADMIN($login_uid)) {
	DOLOGOUT($login_uid);
	$login_status = CHECKLOGIN_NOTLOGGEDIN;
	$login_uid    = 0;
    }
    
    header("Last-Modified: " . gmdate("D, d M Y H:i:s") . " GMT");
    
    if (1) {
	header("Expires: Mon, 26 Jul 1997 05:00:00 GMT");
	header("Cache-Control: no-cache, must-revalidate");
	header("Pragma: no-cache");
    }
    else {
	header("Expires: " . gmdate("D, d M Y H:i:s", time() + 300) . " GMT"); 
    }
?><!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" 
 "http://www.w3.org/TR/html4/loose.dtd">
<html>
  <head>
    <title><?php echo "emulab.net - $title"; ?></title>
    <!-- dumbed-down style sheet for any browser that groks (eg NS47). -->
    <link REL="stylesheet" HREF="<?php echo "$BASEPATH/common-style.css" ?>" TYPE="text/css" />
    <!-- don't import full style sheet into NS47, since it does a bad job
         of handling it. NS47 doesn't understand '@import'. -->
    <style type="text/css" media="all"><!-- @import "<?php echo "$BASEPATH/style.css" ?>"; --></style>
<?php if ($TBMAINSITE) { ?>
    <meta NAME="keywords" CONTENT="network, emulation, internet, emulator">
    <meta NAME="ROBOTS" CONTENT="NOARCHIVE">
<?php } ?>
  </head>
  <body bgcolor="#FFFFFF" topmargin=0 leftmargin=0 marginheight=0 marginwidth=0>
    <table cellpadding="0" cellspacing="0" width="100%">
      <tr>
        <td valign="top" class="bannercell" background="<?php echo "$BASEPATH/headerbgbb.jpg" ?>"
            bgcolor=#3D627F ><img width=369 height=100 src="<?php echo "$BASEPATH/overlay.gif" ?>" 
            alt="emulab - the utah network testbed" /></td>
      </tr>
    </table><table cellpadding="8" cellspacing="0" height="100%">
      <tr height=100%>
        <td valign="top" class="leftcell" bgcolor=#ccddee>
<!-- sidebar begins -->
<?php WRITESIDEBAR(); ?>
<!-- sidebar ends -->
        </td>
        <td valign="top" width=* class="rightcell">
          <table class="content" width=100% cellpadding="0" cellspacing="0">
            <tr>
              <td class="contentheader"><h2 class="nomargin"><?php
    if ($login_uid && ISADMININSTRATOR()) {
	if (ISADMIN($login_uid)) {
	    echo "<a href=adminmode.php3?target_uid=$login_uid&adminoff=1>
	             <img src='/redball.gif'
                          border=0 alt='Admin On'></a>\n";
	}
	else {
	    echo "<a href=adminmode.php3?target_uid=$login_uid&adminoff=0>
	             <img src='/greenball.gif'
                          border=0 alt='Admin Off'></a>\n";
	}
    }
    echo "$title";
?></h2></td>
            </tr>
            <tr>
              <td class="contentbody" width=*>
<!-- begin content -->
<?php
}

#
# ENDPAGE(): This terminates the table started above.
# 
function ENDPAGE() {

?>
              </td>
            </tr>
          </table>
        </td>
      </tr>
    </table>
<?php
}

#
# Spit out a vanilla page footer.
#
function PAGEFOOTER() {
    global $TBDOCBASE, $TBMAILADDR, $THISHOMEBASE;
    global $TBMAINSITE, $SSL_PROTOCOL;

#    echo "<!-- Force full window! -->
#	  <base target=_top>
#          <center>[<a href=\"$TBDOCBASE\">$THISHOMEBASE Home</a>]</center>
#          <center>
#           [<a href=\"http://www.cs.utah.edu/flux/\">Flux Research Group</a>]
#           [<a href=\"http://www.cs.utah.edu/\">School of Computing</a>]
#           [<a href=\"http://www.utah.edu/\">University of Utah</a>]
#          </center>

# echo      "</td></tr><tr><td class=\"contentbody\">"; # new row.

?>
<!-- end content -->
              </td>
            </tr><tr>
              <td class="contentbody">
                <!-- begin copyright -->
                <p align=right>
                  <font size=-2>
                    &copy; 2000-2002 
                    <a href="http://www.utah.edu/">University of Utah</a>
                    and the <a href="http://www.cs.utah.edu/flux/">Flux Group</a>.
                    <a href="<?php echo "$TBDOCBASE"; ?>/docwrapper.php3?docname=copyright.html">
                    All rights reserved.</a>
                  </font>
                  <br />
		  <font size=-2>
                    Problems?
	            Contact <?php echo "$TBMAILADDR\n"; ?>
                  </font>
                </p>
                <!-- end copyright -->
<?php

#echo      "<p align=right>
#            <font size=-1>
#              &copy; 2000-2002 
#              <a href=\"http://www.utah.edu/\">University of Utah</a>
#              and the <a href=\"http://www.cs.utah.edu/flux/\">Flux Group</a>.
#                <a href='$TBDOCBASE/docwrapper.php3?docname=copyright.html'>
#                  All rights reserved.</a>
#           </font><br />\n";

    ENDPAGE();

    if ($TBMAINSITE) {
	echo "<p>
              <a href=\"$TBDOCBASE/netemu.php3\"></a>\n";#

	if (! isset($SSL_PROTOCOL)) {
	    echo "<a href=http://www.addme.com>
	             <img width=8 height=2
	                 src='http://www.addme.com/link8.gif'
   	                 alt='Add Me!' border=0>
	          </a>\n";
	}
    }

?>
  </body>
</html>
<?php
}

function PAGEERROR($msg) {
    global $drewheader;

    if (! $drewheader)
	PAGEHEADER("");

    echo "$msg\n";

    PAGEFOOTER();
    die("");
}

#
# Sub Page/Menu Stuff
#
function WRITESUBMENUBUTTON($text, $link) {


    echo "<!-- Table row for button $text -->
          <tr>
            <td valign=center align=left nowrap>
                <b>
         	 <a class=sidebarbutton href='$link'>$text</a>\n";
    #
    # XXX these blanks look bad in lynx, but add required
    #     spacing between menu and body
    #
#    echo "       &nbsp;&nbsp;\n";

    echo "      </b>
            </td>
          </tr>\n";
}

#
# Start/End a page within a page. 
#
function SUBPAGESTART() {
    echo "<!-- begin subpage -->";
    echo "<table class=\"stealth\" cellspacing=0 cellpadding=0 width='85%' border=0>\n
            <tr>\n
              <td class=\"stealth\"valign=top>\n";
}

function SUBPAGEEND() {
    echo "    </td>\n
            </tr>\n
          </table>\n";
    echo "<!-- end subpage -->";
}

#
# Start/End a sub menu, located in the upper left of the main frame.
# Note that these cannot be used outside of the SUBPAGE macros above.
#
function SUBMENUSTART($title) {
?>
    <!-- begin submenu -->
    <table class="menu" width=200 cellpadding="0" cellspacing="0">
      <tr>
        <td class="menuheader"><b><?php echo "$title";?></b></td>
      </tr>
<?php
}

function SUBMENUEND() {
?>
    </table>
    <!-- end submenu -->
  </td>
  <td class="stealth" valign=top align=left width='85%'>
<?php
}

?>