<?php

$STATUS_NOSTATUS  = 0;
$STATUS_LOGGEDIN  = 1;
$STATUS_LOGGEDOUT = 2;
$STATUS_LOGINFAIL = 3;
$STATUS_TIMEDOUT  = 4;
$login_status     = $STATUS_NOSTATUS;
$login_message    = "";
$error_message    = 0;

#
# WRITESIDEBARBUTTON(text, link): Write a button on the sidebar menu.
# We do not currently try to match the current selection so that its
# link looks different. Not sure its really necessary.
#
function WRITESIDEBARBUTTON($text, $base, $link) {
    $link = "$base" . "$link";
    
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
    global $login_status, $login_message, $error_message, $uid, $TBDBNAME;
    global $STATUS_NOSTATUS, $STATUS_LOGGEDIN, $STATUS_LOGGEDOUT;
    global $STATUS_LOGINFAIL, $STATUS_TIMEDOUT;
    global $TBBASE, $TBDOCBASE;

    echo "<table cellspacing=2 cellpadding=2 border=0 width=150>\n";

    WRITESIDEBARBUTTON("Home", $TBBASE, "index.php3");
    WRITESIDEBARBUTTON("Publications", $TBDOCBASE, "pubs.php3");
    WRITESIDEBARBUTTON("Documentation", $TBDOCBASE, "doc.php3");
    WRITESIDEBARBUTTON("FAQ", $TBDOCBASE, "faq.php3");
    WRITESIDEBARBUTTON("Tutorial", $TBDOCBASE, "tutorial/tutorial.php3");
    WRITESIDEBARBUTTON("People", $TBDOCBASE, "people.php3");

    echo "<tr>
            <td height=30 valign=center align=center nowrap>
             <b><span class=sidebarbutton>
                  Web Interface Options
                </span>
             </b>
            </td>
          </tr>\n";

    if ($login_status == $STATUS_LOGGEDIN) {
	$query_result = mysql_db_query($TBDBNAME,
		"SELECT status,admin,stud FROM users WHERE uid='$uid'");
	$row = mysql_fetch_row($query_result);
	$status = $row[0];
	$admin  = $row[1];
	$stud   = $row[1];

        #
        # See if group_root in any projects, not just the last one in the DB!
        #
	$query_result = mysql_db_query($TBDBNAME,
		"SELECT trust FROM proj_memb ".
		"WHERE uid='$uid' and trust='group_root'");
	if (mysql_num_rows($query_result)) {
	    $trusted = 1;
	}
	else {
	    $trusted = 0;
	}

	if ($status == "active") {
	    if ($admin) {
		WRITESIDEBARBUTTON("New Project Approval",
				   $TBBASE, "approveproject_list.php3");
		WRITESIDEBARBUTTON("Node Control",
				   $TBBASE, "nodecontrol_list.php3");
		WRITESIDEBARBUTTON("User List",
				   $TBBASE, "showuser_list.php3");
	    }
	    if ($trusted) {
                # Only group leaders can do these options
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
	    WRITESIDEBARBUTTON("Begin an Experiment",
			       $TBBASE, "beginexp_form.php3");
	    WRITESIDEBARBUTTON("Experiment Information",
			       $TBBASE, "showexp_list.php3");

	    if ($stud) {
		WRITESIDEBARBUTTON("Create a Batch Experiment",
				   $TBBASE, "batchexp_form.php3");
	    }
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
    WRITESIDEBARBUTTON("Start Project", $TBBASE, "newproject_form.php3");
    WRITESIDEBARBUTTON("Join Project",  $TBBASE, "addusr.php3");

    switch ($login_status) {
    case $STATUS_LOGGEDIN:
	$login_message = "$uid Logged In";
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
    }

    #
    # Now the login/logout box. Remember, already inside a table.
    # 
    echo "<form action=\"index.php3\" method=post>\n";
    if ($login_status == $STATUS_LOGGEDIN) {
	echo "<tr>
                <td><input type=hidden name=uid value=\"$uid\"></td>
              </tr>
              <tr>
                <td align=center>
                    <b><input type=submit value=Logout name=logout></b>
                </td>
              </tr>\n";
    }
    else {
	#
	# Get the UID that came back in the cookie so that we can present a
	# default login name to the user.
	#
	if (($uid = GETUID()) == FALSE) {
	    $uid = "";
	}

	echo "<tr>
                <td>
                   Username:<br>
                            <input type=text value=\"$uid\" name=uid size=8>
                </td>
              </tr>
              <tr>
                <td>
                   Password:<br>
                            <input type=password name=password size=12>
                </td>
              </tr>
              <tr>
                <td align=center>
                    <b><input type=submit value=Login name=login></b>
                </td>
              </tr>\n";
    }
    echo "</form>\n";
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
    global $login_status, $TBAUTHTIMEOUT, $uid;
    global $STATUS_NOSTATUS, $STATUS_LOGGEDIN, $STATUS_LOGGEDOUT;
    global $STATUS_LOGINFAIL, $STATUS_TIMEDOUT;
    global $TBBASE, $TBDOCBASE;

    if ($login_status == $STATUS_NOSTATUS) {
	if ($uid = GETUID()) {
            #
            # Check to make sure the UID is logged in (not timed out).
            #
            $status = CHECKLOGIN($uid);
            switch ($status) {
	    case 0:
		unset($uid);
		break;
	    case 1:
		$login_status = $STATUS_LOGGEDIN;
		break;
	    case -1:
		$login_status = $STATUS_TIMEDOUT;
		unset($uid);
		break;
	    }
	}
    }

    echo "<html>
          <head>
           <title>Emulab.Net - $title</title>\n";

    #
    # Shove the Jave code through to the output. 
    # 
    readfile("java.html");

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
                  function doLoad() {
                    setTimeout(\"refresh()\", $timeo );
                  }
                //-->
              </script>\n";
    }
    
    echo " <link rel=\"stylesheet\" href=\"$TBDOCBASE/tbstyle.css\"
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
    global $TBBASE;

    ENDPAGE();

    echo "<!-- Force full window! -->
	  <base target=_top>
          <center>[<a href=\"$TBBASE\">Emulab.Net Home</a>]</center>
          <center>
           [<a href=\"http://www.cs.utah.edu/flux/\">Flux Research Group</a>]
           [<a href=\"http://www.cs.utah.edu/\">School of Computing</a>]
           [<a href=\"http://www.utah.edu/\">University of Utah</a>]
          </center>
         <p align=right>
         <font size=-2>
          Problems? Contact
                    <a href=\"mailto:testbed-ops@flux.cs.utah.edu\"> 
                       Testbed Operations (testbed-ops@flux.cs.utah.edu)</a>
          </body>
          </html>\n";
}
?>
