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
function WRITESIDEBARBUTTON($text, $base, $link) {
    $link = "$base/$link";
    
    echo "<!-- Table row for button $text -->
          <tr>
            <td valign=center align=left nowrap>
                <b>
         	 <a class=sidebarbutton href=\"$link\">$text</a>\n";
    #
    # XXX these blanks look bad in lynx, but add required
    #     spacing between menu and body
    #
    echo "       &nbsp;&nbsp;\n";

    echo "      </b>
            </td>
          </tr>\n";
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
    echo "<table cellspacing=2 cellpadding=2 border=0 width=150>\n";

    WRITESIDEBARBUTTON("Home", $TBDOCBASE, "index.php3");
    WRITESIDEBARBUTTON("Tutorial at SIGCOMM'02 <img src=/new.gif>",
		       $TBDOCBASE, "sc2002tut.php3");
    WRITESIDEBARBUTTON("News (Updated Apr 19)", $TBDOCBASE,
		       "docwrapper.php3?docname=news.html");
    WRITESIDEBARBUTTON("Tutorial", $TBDOCBASE, "tutorial/tutorial.php3");
    WRITESIDEBARBUTTON("FAQ", $TBDOCBASE, "faq.php3");
    WRITESIDEBARBUTTON("Documentation", $TBDOCBASE, "doc.php3");
    WRITESIDEBARBUTTON("Search Documentation", $TBDOCBASE, "search.php3");
    WRITESIDEBARBUTTON("Papers <img src=/new.gif>", $TBDOCBASE, "pubs.php3");
    WRITESIDEBARBUTTON("People", $TBDOCBASE, "people.php3");
    WRITESIDEBARBUTTON("The Gallery", $TBDOCBASE, "gallery/gallery.php3");
    WRITESIDEBARBUTTON("Projects Using $THISHOMEBASE", $TBDOCBASE,
		       "projectlist.php3");
    WRITESIDEBARBUTTON("Sponsors", $TBDOCBASE,
		       "docwrapper.php3?docname=sponsors.html");

    echo "<tr>
            <td height=30 valign=center align=center nowrap>
             <b><span class=sidebarbutton>
                  Web Interface Options\n";

    if ($login_status & CHECKLOGIN_LOGGEDIN) {
	$freepcs = TBFreePCs();
	
	echo "    <br>($freepcs Free PCs)\n";
    }
    echo "      </span>
             </b>
            </td>
          </tr>\n";

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
	    echo "<tr>
                    <td height=30 valign=center align=center nowrap>
                    <b><span class=sidebarbutton>
                         Please Try Again Later
                       </span>
                    </b>
                 </td>
                </tr>\n";
	}
    }
    if ($login_status & (CHECKLOGIN_LOGGEDIN|CHECKLOGIN_MAYBEVALID)) {
	if ($login_status & CHECKLOGIN_ACTIVE) {
	    if ($login_status & CHECKLOGIN_PSWDEXPIRED) {
		WRITESIDEBARBUTTON("Change Your Password",
				   $TBBASE, "moduserinfo.php3");
	    }
	    else {
		WRITESIDEBARBUTTON("My $THISHOMEBASE",
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
	WRITESIDEBARBUTTON("Join Project",  $TBBASE, "joinproject.php3");
    }

    #
    # Cons up a nice message.
    # 
    switch ($login_status & CHECKLOGIN_STATUSMASK) {
    case CHECKLOGIN_LOGGEDIN:
	$login_message = "$login_uid Logged In";
	    
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

    #
    # Now the login/logout box. Remember, already inside a table.
    # We want the links to the login/logout pages to always be https,
    # but the images path depends on whether the page was loaded as
    # http or https, since we do not want to mix them, since they
    # cause warnings.
    # 
    if ($login_status & (CHECKLOGIN_LOGGEDIN|CHECKLOGIN_MAYBEVALID)) {
	echo "<tr>
               <td align=center height=50 valign=center>
                <a href=\"$TBBASE/logout.php3?uid=$login_uid\">
	           <img alt=\"logout\" border=0
                        src=\"$BASEPATH/logoff.gif\"></a>
               </td>
              </tr>\n";
    }
    elseif (!NOLOGINS()) {
	echo "<tr>
               <td align=center height=50 valign=center>
                <a href=\"$TBBASE/login.php3\">
	           <img alt=\"logon\" border=0
                        src=\"$BASEPATH/logon.gif\"></a>
               </td>
              </tr>\n";
    }

    if ($login_message) {
	echo "<tr>
                <td align=center>
                 <b>
                   <span class=sidebarbutton>
                      $login_message
                   </span>
                 <b>
                </td>
              </tr>\n";
    }

    #
    # MOTD. Set this with the webcontrol script.
    #
    $query_result =
	DBQueryFatal("SELECT message FROM loginmessage");
    
    if (mysql_num_rows($query_result)) {
    	$row = mysql_fetch_row($query_result);
	$message = $row[0];
    
	echo "<tr>
                <td align=center>
                 <b>
                   <span class=sidebarbutton>
                    <font size=\"+1\" color=RED>
                      $message
                    </font>
                   </span>
                 <b>
                </td>
              </tr>\n";
    }
    echo "</table>
          <br>\n";
}

#
# WRITEBANNER(): write the page banner for a page.
# Called by _STARTPAGE
#
function WRITEBANNER($title) {
    global $BANNERCOLOR, $THISPROJECT, $THISHOMEBASE, $BASEPATH;
    global $login_uid;

    echo "<!-- This is the page Banner -->\n";

    echo "<table cellpadding=0 cellspacing=0 border=0 width=50%>";
    echo "<tr>
            <td align=left valign=top width=0%>
             <table cellpadding=5 cellspacing=0 border=0 bgcolor=\"#880000\">
              <tr>
                <td>
                 <b><font size=5 color=white face=Helvetica>
                        $THISHOMEBASE
                    </font>
                 </b>
                </td>
              </tr>
             </table>
            </td>
            <td align=left valign=top width=70%>
                <table cellpadding=5 cellspacing=0 border=0
                         bgcolor=$BANNERCOLOR>
                  <tr>
                   <td nowrap>
                     <b>
                       <font size=5 face=helvetica color=\"#000000\">
                            $THISPROJECT
                       </font>
                     </b>
                   </td>
                  <tr>
                </table>
            </td>
            <td align=left valign=center width=50%>
                <table cellpadding=0 cellspacing=0 border=0>
                  <tr>
                   <td nowrap align=right>\n";
    if ($login_uid && ISADMININSTRATOR()) {
	if (ISADMIN($login_uid)) {
	    echo "<a href=adminmode.php3?target_uid=$login_uid&adminoff=1>
	             <img src='/autostatus-icons/redball.gif'
                          border=0 alt='Admin On'></a>\n";
	}
	else {
	    echo "<a href=adminmode.php3?target_uid=$login_uid&adminoff=0>
	             <img src='/autostatus-icons/greenball.gif'
                          border=0 alt='Admin Off'></a>\n";
	}
    }
    else {
	echo "       <b>
                       <font size=5 face=helvetica color=\"#000000\">
                            &nbsp
                       </font>
                     </b>\n";
    }
    echo "         </td>
                  <tr>
                </table>
            </td>
          </tr>
          </table>\n";
}

#
# _WRITETITLE(title): write the page title for a page.
# Called by _STARTPAGE
#
function WRITETITLE($title) {
    global $TITLECOLOR, $BASEPATH;
    
    echo "<!-- This is the page Title -->
          <table width=\"100%\">
            <tr>
               <td width=\"10%\" align=left>\n";
                 #
                 # Insert a small logo here if you like.
                 #
    
    echo "     </td>
               <td align=left width=\"90%\">
                   <b><font size=5 color=$TITLECOLOR>$title</font></b>
               </td>
            </tr>
          </table>\n";
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

    echo "<html>
          <head>
           <title>$THISHOMEBASE - $title</title>\n";

    if ($TBMAINSITE) {
	echo "  <meta NAME=\"keywords\"
                      CONTENT=\"network, emulation, internet, emulator\">
                <meta NAME=\"ROBOTS\" CONTENT=\"NOARCHIVE\">\n";
    }
    
    echo " <link rel=\"stylesheet\" href=\"$BASEPATH/tbstyle.css\"
                 type=\"text/css\">
          </head>\n";
    echo "<body>\n";

    WRITEBANNER($title);
    WRITETITLE($title);

    echo "<br>
          <!-- Under banner is a table with two columns: menu and content -->
          <table cellpadding=3 width=\"100%\" height=\"60%\" border=2>
          <tr>
           <!-- everthing is in a single row: two tables side-by-side -->

           <!-- MENU -->
           <td valign=top>\n";

    WRITESIDEBAR();

    echo " </td>

           <!-- PAGE BODY -->
           <td valign=top align=left width=\"85%\">

               <!-- Content follows this macro ... -->\n";
}

#
# ENDPAGE(): This terminates the table started above.
# 
function ENDPAGE() {
    echo "     <!-- ... end of page content. -->
            </td>
           </tr>
          </table>
          <br>
          <hr size=2 noshade>\n";
}

#
# Spit out a vanilla page footer.
#
function PAGEFOOTER() {
    global $TBDOCBASE, $TBMAILADDR, $THISHOMEBASE;
    global $TBMAINSITE, $SSL_PROTOCOL;

    ENDPAGE();

    echo "<!-- Force full window! -->
	  <base target=_top>
          <center>[<a href=\"$TBDOCBASE\">$THISHOMEBASE Home</a>]</center>
          <center>
           [<a href=\"http://www.cs.utah.edu/flux/\">Flux Research Group</a>]
           [<a href=\"http://www.cs.utah.edu/\">School of Computing</a>]
           [<a href=\"http://www.utah.edu/\">University of Utah</a>]
          </center>
          <center>
           <font size=-1>
             &copy; 2000-2002 University of Utah and the Flux Group.
               <a href='$TBDOCBASE/docwrapper.php3?docname=copyright.html'>
                  All rights reserved.</a>
           </font>
          </center>
         <p align=right>
         <font size=-2>
          Problems? Contact $TBMAILADDR\n";

    if ($TBMAINSITE) {
	echo "<p>
              <a href=\"$TBDOCBASE/netemu.php3\"></a>\n";

	if (! isset($SSL_PROTOCOL)) {
	    echo "<a href=http://www.addme.com>
	             <img width=8 height=2
	                 src='http://www.addme.com/link8.gif'
   	                 alt='Add Me!' border=0>
	          </a>\n";
	}
    }

    echo "</font>
          </body>
          </html>\n";
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
    echo "       &nbsp;&nbsp;\n";

    echo "      </b>
            </td>
          </tr>\n";
}

#
# Start/End a page within a page. 
#
function SUBPAGESTART() {
    echo "<table cellspacing=2 cellpadding=2 width='85%' border=0>\n
            <tr>\n
              <td valign=top>\n";
}

function SUBPAGEEND() {
    echo "    </td>\n
            </tr>\n
          </table>\n";
}

#
# Start/End a sub menu, located in the upper left of the main frame.
# Note that these cannot be used outside of the SUBPAGE macros above.
#
function SUBMENUSTART($title) {
    echo "      <table cellspacing=2 cellpadding=2 border=0 width=200>\n
                  <tr>\n
                    <td align=center><b>$title</b></td>\n
                  </tr>\n
                  <tr></tr>\n";
}

function SUBMENUEND() {
    echo "      </table>\n
              </td>\n
              <td valign=top align=left width='85%'>\n";
}

?>
