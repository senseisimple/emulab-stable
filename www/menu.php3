<?php

$STATUS_NOSTATUS  = 0;
$STATUS_LOGGEDIN  = 1;
$STATUS_LOGGEDOUT = 2;
$STATUS_LOGINFAIL = 3;
$STATUS_TIMEDOUT  = 4;
$STATUS_NOLOGINS  = 5;
$login_status     = $STATUS_NOSTATUS;
$pswd_expired     = 0;
$login_message    = "";
$error_message    = 0;
$login_uid        = 0;
$drewheader       = 0;
$javacode         = file("java.html");

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
    global $login_status, $login_message, $error_message, $login_uid;
    global $STATUS_NOSTATUS, $STATUS_LOGGEDIN, $STATUS_LOGGEDOUT;
    global $STATUS_LOGINFAIL, $STATUS_TIMEDOUT, $STATUS_NOLOGINS;
    global $TBBASE, $TBDOCBASE, $TBDBNAME, $BASEPATH, $pswd_expired;

    #
    # The document base cannot be a mix of secure and nonsecure.
    # 

    echo "<table cellspacing=2 cellpadding=2 border=0 width=150>\n";

    WRITESIDEBARBUTTON("Home", $TBDOCBASE, "index.php3");
    WRITESIDEBARBUTTON("News (new Nov 29)", $TBDOCBASE,
		       "docwrapper.php3?docname=news.html");
    WRITESIDEBARBUTTON("Publications", $TBDOCBASE, "pubs.php3");
    WRITESIDEBARBUTTON("Documentation", $TBDOCBASE, "doc.php3");
    WRITESIDEBARBUTTON("Search Documentation", $TBDOCBASE, "search.php3");
    WRITESIDEBARBUTTON("FAQ", $TBDOCBASE, "faq.php3");
    WRITESIDEBARBUTTON("Tutorial", $TBDOCBASE, "tutorial/tutorial.php3");
    WRITESIDEBARBUTTON("People", $TBDOCBASE, "people.php3");
    WRITESIDEBARBUTTON("The Gallery", $TBDOCBASE, "gallery/gallery.php3");
    WRITESIDEBARBUTTON("Projects Using Emulab.Net", $TBDOCBASE,
		       "projectlist.php3");

    echo "<tr>
            <td height=30 valign=center align=center nowrap>
             <b><span class=sidebarbutton>
                  Web Interface Options
                </span>
             </b>
            </td>
          </tr>\n";

    if ($login_status == $STATUS_NOLOGINS) {
        WRITESIDEBARBUTTON("Web Interface Temporarily Unavailable",
			   $TBDOCBASE, "nologins.php3");

	echo "<tr>
               <td height=30 valign=center align=center nowrap>
                  <b><span class=sidebarbutton>
                       Please Try Again Later
                     </span>
                  </b>
               </td>
              </tr>\n";
    }
    elseif ($login_status == $STATUS_LOGGEDIN) {
	$query_result =
	    DBQueryFatal("SELECT status,admin ".
			 "FROM users WHERE uid='$login_uid'");
	$row = mysql_fetch_row($query_result);
	$status = $row[0];
	$admin  = $row[1];

        #
        # See if group_root in any projects, not just the last one in the DB!
        #
	$query_result = mysql_db_query($TBDBNAME,
		"SELECT trust FROM group_membership ".
		"WHERE uid='$login_uid' and ".
		"      (trust='group_root' or trust='project_root')");
	if (mysql_num_rows($query_result)) {
	    $trusted = 1;
	}
	else {
	    $trusted = 0;
	}

	if ($status == "active" && $pswd_expired) {
	    WRITESIDEBARBUTTON("Change your Password",
			       $TBBASE, "modusr_form.php3");
	}
	elseif ($status == "active") {
	    if ($admin) {
		WRITESIDEBARBUTTON("New Project Approval",
				   $TBBASE, "approveproject_list.php3");
	    }
	    if ($trusted) {
                # Only project leaders can do these options
		WRITESIDEBARBUTTON("New User Approval",
				   $TBBASE, "approveuser_form.php3");
	    }
	    #
            # Since a user can be a member of more than one project,
            # display this option, and let the form decide if the user is
            # allowed to do this.
	    #
	    WRITESIDEBARBUTTON("Project Information",
			       $TBBASE, "showproject_list.php3");
	    
	    if ($admin) {
		WRITESIDEBARBUTTON("User List",
				   $TBBASE, "showuser_list.php3");
		WRITESIDEBARBUTTON("Node Control",
				   $TBBASE, "nodecontrol_list.php3");
	    }
	    
	    WRITESIDEBARBUTTON("Begin an Experiment",
			       $TBBASE, "beginexp_form.php3");
	    WRITESIDEBARBUTTON("Experiment Information",
			       $TBBASE, "showexp_list.php3");
	    WRITESIDEBARBUTTON("Create a Batch Experiment",
			       $TBBASE, "batchexp_form.php3");
	    WRITESIDEBARBUTTON("Update user information",
			       $TBBASE, "modusr_form.php3");
	    WRITESIDEBARBUTTON("Node Reservation Status",
			       $TBBASE, "reserved.php3");
	    WRITESIDEBARBUTTON("Node Up/Down Status",
			       $TBDOCBASE, "updown.php3");
                    
	}
	elseif (($status == "newuser") || ($status == "unverified")) {
	    WRITESIDEBARBUTTON("New User Verification",
			       $TBBASE, "verifyusr_form.php3");
	}
	elseif ($status == "unapproved") {
	    $error_message = "Your account has not been approved yet. ".
		             "Please try back later";
	}
    }
    #
    # Standard options for anyone.
    #
    if ($login_status != $STATUS_NOLOGINS) {
	WRITESIDEBARBUTTON("Start Project", $TBBASE, "newproject_form.php3");
	WRITESIDEBARBUTTON("Join Project",  $TBBASE, "addusr.php3");
    }

    switch ($login_status) {
    case $STATUS_LOGGEDIN:
	$login_message = "$login_uid Logged In";
	if ($pswd_expired)
	    $login_message = "$login_message<br>(Password Expired!)";
	break;
    case $STATUS_LOGGEDOUT:
	$login_message = "Logged Out";
	break;
    case $STATUS_LOGINFAIL:
	$login_message = "Login Failed";
	break;
    case $STATUS_TIMEDOUT:
	$login_message = "Login Timed Out";
	break;
    case $STATUS_NOLOGINS:
	$login_message = "Please Try Again Later";
	break;
    }

    #
    # Now the login/logout box. Remember, already inside a table.
    # We want the links to the login/logout pages to always be https,
    # but the images path depends on whether the page was loaded as
    # http or https, since we do not want to mix them, since they
    # cause warnings.
    # 
    if ($login_status == $STATUS_LOGGEDIN) {
	echo "<tr>
               <td align=center height=50 valign=center>
                <a href=\"$TBBASE/logout.php3?uid=$login_uid\">
	           <img alt=\"logout\" border=0
                        src=\"$BASEPATH/logoff.gif\"></a>
               </td>
              </tr>\n";
    }
    else {
	echo "<tr>
               <td align=center height=50 valign=center>
                <a href=\"$TBBASE/login_form.php3\">
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
    # The blinking is for Mike, who says he really likes it. 
    #
    $query_result = mysql_db_query($TBDBNAME,
	"SELECT message FROM loginmessage");
    
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
    global $BANNERCOLOR, $THISPROJECT, $THISHOMEBASE;

    echo "<!-- This is the page Banner -->\n";
    echo "<table cellpadding=0 cellspacing=0 border=0 bgcolor=$BANNERCOLOR>";
    echo "<tr>
            <td align=left width=\"0%\">
             <table cellpadding=5 cellspacing=0 border=0 bgcolor=\"#880000\">
              <tr>
                <td>
                 <b><font size=15 color=white face=Helvetica>
                     $THISHOMEBASE
                    </font>
                 </b>
                </td>
              </tr>
             </table>
            </td>
            <td align=left width=\"100%\">
                <table cellpadding=5 cellspacing=0 border=0
                         bgcolor=$BANNERCOLOR>
                  <tr>
                   <td>
                     <b>
                       <font size=15 face=helvetica color=\"#000000\">
                            $THISPROJECT
                       </font>
                     </b>
                   </td>
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
    global $TITLECOLOR;
    
    echo "<!-- This is the page Title -->
          <table width=\"100%\">
            <tr>
               <td width=\"15%\" align=left>\n";
                 #
                 # Insert a small logo here if you like.
                 #
    echo "     </td>
               <td align=left>
                   <b><font size=\"+3\" color=$TITLECOLOR>$title</font></b>
               </td>
            </tr>
          </table>\n";
}

#
# Spit out a vanilla page header.
#
function PAGEHEADER($title) {
    global $login_status, $TBAUTHTIMEOUT, $login_uid;
    global $STATUS_NOSTATUS, $STATUS_LOGGEDIN, $STATUS_LOGGEDOUT;
    global $STATUS_LOGINFAIL, $STATUS_TIMEDOUT, $STATUS_NOLOGINS;
    global $TBBASE, $TBDOCBASE, $TBDBNAME;
    global $CHECKLOGIN_NOTLOGGEDIN, $CHECKLOGIN_LOGGEDIN;
    global $CHECKLOGIN_TIMEDOUT, $CHECKLOGIN_MAYBEVALID;
    global $CHECKLOGIN_PSWDEXPIRED;
    global $BASEPATH, $SSL_PROTOCOL, $javacode, $drewheader, $pswd_expired;

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
        $status = CHECKLOGIN($known_uid);
	switch ($status) {
	case $CHECKLOGIN_NOTLOGGEDIN:
	    $login_status = $STATUS_NOSTATUS;
	    $login_uid    = 0;
	    break;
	case $CHECKLOGIN_PSWDEXPIRED:
	    $pswd_expired = 1;
	case $CHECKLOGIN_LOGGEDIN:
	case $CHECKLOGIN_MAYBEVALID:
	    $login_status = $STATUS_LOGGEDIN;
	    $login_uid    = $known_uid;
	    break;
	case $CHECKLOGIN_TIMEDOUT:
	    $login_status = $STATUS_TIMEDOUT;
	    $login_uid    = 0;
	    break;
	}
    }

    #
    # Check for NOLOGINS. This is complicated by the fact that we
    # want to allow admin types to continue using the web interface,
    # and logout anyone else that is currently logged in!
    #
    if ($login_status == $STATUS_LOGGEDIN && NOLOGINS()) {
	$query_result = mysql_db_query($TBDBNAME,
		"SELECT admin FROM users WHERE uid='$login_uid'");
	$row = mysql_fetch_row($query_result);
	$admin  = $row[0];

	if (!$admin) {
	    DOLOGOUT($login_uid);
	    $login_status = $STATUS_NOLOGINS;
	}
    }
    elseif (NOLOGINS()) {
	$login_status = $STATUS_NOLOGINS;
    }

    echo "<html>
          <head>
           <title>Emulab.Net - $title</title>\n";

    #
    # If logged in, initialize the refresh process.
    # 
    if ($login_status == $STATUS_LOGGEDIN) {
	$timeo = $TBAUTHTIMEOUT + 120;
	echo "<noscript>
                <META HTTP-EQUIV=\"refresh\" CONTENT=\"$timeo\">
              </noscript>\n";
	$timeo *= 1000;
	echo "<script language=\"JavaScript\">
                <!--
                  var sURL = \"$TBDOCBASE/index.php3\";

                  function doLoad() {
                    setTimeout(\"refresh()\", $timeo );
                  }
                //-->
              </script>\n";
        #
        # Shove the rest of the Java code through to the output. 
        #
	for ($i = 0; $i < count($javacode); $i++) {
	    echo "$javacode[$i]";
	}
    }
    
    echo " <meta HTTP-EQUIV=\"Pragma\" CONTENT=\"no-cache\">
           <link rel=\"stylesheet\" href=\"$BASEPATH/tbstyle.css\"
                 type=\"text/css\">
          </head>\n";

    #
    # If logged in, start the refresh process.
    # 
    if ($login_status == $STATUS_LOGGEDIN) {
	echo "<body onload=\"doLoad()\">\n";
    }
    else {
	echo "<body>\n";
    }

    echo "<basefont size=4>\n";

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
               <basefont size=4>
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
    global $TBDOCBASE, $TBMAILADDR;

    ENDPAGE();

    echo "<!-- Force full window! -->
	  <base target=_top>
          <center>[<a href=\"$TBDOCBASE\">Emulab.Net Home</a>]</center>
          <center>
           [<a href=\"http://www.cs.utah.edu/flux/\">Flux Research Group</a>]
           [<a href=\"http://www.cs.utah.edu/\">School of Computing</a>]
           [<a href=\"http://www.utah.edu/\">University of Utah</a>]
          </center>
         <p align=right>
         <font size=-2>
          Problems? Contact $TBMAILADDR
          </font>
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
?>
