<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
# All rights reserved.
#

$login_status     = CHECKLOGIN_NOTLOGGEDIN;
$login_uid        = 0;
$drewheader       = 0;
$noheaders	  = 0;
$autorefresh      = 0;

#
# This has to be set so we can spit out http or https paths properly!
# Thats because browsers do not like a mix of secure and nonsecure.
# 
$BASEPATH	  = "";

#
# TOPBARCELL - Make a cell for the topbar. Actually, the name lies, it can be
# used for cells in a bottombar too.
#
function TOPBARCELL($contents) {
    echo "<td class=\"topbaropt\">";
    echo "<span class=\"topbaroption\">&nbsp;";
    echo $contents;
    echo "&nbsp;</span>";
    echo "</td>";
    echo "\n";
}

#
# SIDEBARCELL - Make a cell for the sidebar
#
function SIDEBARCELL($contents, $last = 0) {
    echo "<tr>";
    if ($last) {
	echo "<td class=\"menuoptb\">";
    } else {
	echo "<td class=\"menuopt\">";
    }
    echo "$contents";
    echo "</td>";
    echo "</tr>";
    echo "\n";
}

#
# WRITETOPBARBUTTON(text, base, link): Write a button in the topbar
#
function WRITETOPBARBUTTON($text, $base, $link ) {
    $link = "$base/$link";
    TOPBARCELL("<a href=\"$link\">$text</a>");
}
# same as above with "new" gif next to it.
function WRITETOPBARBUTTON_NEW($text, $base, $link ) {
    $link = "$base/$link";
    TOPBARCELL("<a href=\"$link\">$text</a>&nbsp;<img src=\"/new.gif\" />");
}

#
# WRITESIDEBARDIVIDER(): Put a bit of whitespace in the sidebar
#
function WRITESIDEBARDIVIDER() {
    global $BASEPATH;
    echo "<tr>";
    echo "<td class=\"menuoptdiv\">";
    # We have to put something in this cell, or IE ignores it. But, we do not
    # want to make the table row full line-height, so a space will not do.
    echo "<img src=\"$BASEPATH/1px.gif\" border=0 height=1 width=1 />";
    echo "</td>";
    echo "</tr>";
    echo "\n";
}

#
# WRITESIDEBARBUTTON(text, link): Write a button on the sidebar menu.
# We do not currently try to match the current selection so that its
# link looks different. Not sure its really necessary.
#
function WRITESIDEBARBUTTON($text, $base, $link ) {
    if ($base)
	$link = "$base/$link";
    SIDEBARCELL("<a href=\"$link\">$text</a>");
}

# same as above with "new" gif next to it.
function WRITESIDEBARBUTTON_NEW($text, $base, $link ) {
    $link = "$base/$link";
    SIDEBARCELL("<a href=\"$link\">$text</a>&nbsp;<img src=\"/new.gif\" />");
}

# same as above with "cool" gif next to it.
function WRITESIDEBARBUTTON_COOL($text, $base, $link ) {
    $link = "$base/$link";
    SIDEBARCELL("<a href=\"$link\">$text</a>&nbsp;<img src=\"/cool.gif\" />");
}

function WRITESIDEBARBUTTON_ABS($text, $base, $link ) {
    $link = "$link";
    SIDEBARCELL("<a href=\"$link\">$text</a>\n");
}

function WRITESIDEBARBUTTON_ABSCOOL($text, $base, $link ) {
    $link = "$link";
    SIDEBARCELL("<a href=\"$link\">$text</a>&nbsp;<img src=\"/cool.gif\" />");
}

# same as above, but uses a slightly different style sheet so there
# is more padding below the last button.
# The devil is, indeed, in the details.
function WRITESIDEBARLASTBUTTON($text, $base, $link) {
    $link = "$base/$link";
    SIDEBARCELL("<a href=\"$link\">$text</a>",1);
}

function WRITESIDEBARLASTBUTTON_COOL($text, $base, $link) {
    $link = "$base/$link";
    SIDEBARCELL("<a href=\"$link\">$text</a>&nbsp;<img src=\"/cool.gif\" />",1);
}

# writes a message to the sidebar, without clickability.
function WRITESIDEBARNOTICE($text) {
    SIDEBARCELL("<b>$text</b>");
}

#
# Something like the sidebar, but across the top, with only a few options.
# Think Google. For PlanetLab users, but it would be easy enough to make
# others. Still a work in progress.
#
function WRITEPLABTOPBAR() {
    echo "<table class=\"topbar\" width=\"100%\" cellpadding=\"2\" cellspacing=\"0\" align=\"center\">\n";
    global $login_status, $login_uid;
    global $TBBASE, $TBDOCBASE, $BASEPATH;
    global $THISHOMEBASE;

    WRITETOPBARBUTTON("Create a Slice",
        $TBBASE, "plab_ez.php3");

    WRITETOPBARBUTTON("Nodes",
        $TBBASE, "plabmetrics.php3");

    WRITETOPBARBUTTON("My Testbed",
	$TBBASE,
	"showuser.php3?target_uid=$login_uid");


    WRITETOPBARBUTTON("Advanced Experiment",
        $TBBASE, "beginexp_html.php3");

    if ($login_status & CHECKLOGIN_TRUSTED) {
	# Only project/group leaders can do these options
	# Show a "new" icon if there are people waiting for approval
	$query_result =
	DBQueryFatal("select g.* from group_membership as authed ".
		     "left join group_membership as g on ".
		     " g.pid=authed.pid and g.gid=authed.gid ".
		     "left join users as u on u.uid=g.uid ".
		     "where u.status!='".
		     TBDB_USERSTATUS_UNVERIFIED . "' and ".
		     " u.status!='" . TBDB_USERSTATUS_NEWUSER . 
		     "' and g.uid!='$login_uid' and ".
		     "  g.trust='". TBDB_TRUSTSTRING_NONE . "' ".
		     "  and authed.uid='$login_uid' and ".
		     "  (authed.trust='group_root' or ".
		     "   authed.trust='project_root') ".
		     "ORDER BY g.uid,g.pid,g.gid");
	if (mysql_num_rows($query_result) > 0) {
	     WRITETOPBARBUTTON_NEW("Approve Users",
				   $TBBASE, "approveuser_form.php3");
	} else {
	    WRITETOPBARBUTTON("Approve Users",
			       $TBBASE, "approveuser_form.php3");
	}
    }

    WRITETOPBARBUTTON("Log Out", $TBBASE, "logout.php3?next_page=" .
	urlencode($TBBASE . "/login_plab.php3"));

    echo "</table>\n";
    echo "<br>\n";

}


#
# Put a few things that PlanetLab users should see, but are non-essential,
# across the bottom of the page rather than the top
#
function WRITEPLABBOTTOMBAR() {
    global $login_status, $login_uid;
    global $TBBASE, $TBDOCBASE, $BASEPATH;
    global $THISHOMEBASE;

    if ($login_uid) {
	$newsBase = $TBBASE; 
    } else {
	$newsBase = $TBDOCBASE;
    }

    echo "
	   <center>
	   <br>
	   <font size=-1>
	   <form method=get action=$TBDOCBASE/search.php3>
	   [ <a href='$TBDOCBASE/doc.php3'>
		Documentation</a> : <input name=query size = 15/>
		  <input type=submit style='font-size:10px;' value='Search' /> ]
	   [ <a href='$newsBase/news.php3'>
		News</a> ]
	   </form>
	   </font>
	   <br>
	   </center>\n";

}

#
# WRITESIDEBAR(): Write the menu. The actual menu options the user
# sees depends on the login status and the DB status.
#
function WRITESIDEBAR() {
    global $login_status, $login_uid, $pid, $gid;
    global $TBBASE, $TBDOCBASE, $BASEPATH, $WIKISUPPORT, $MAILMANSUPPORT;
    global $BUGDBSUPPORT, $BUGDBURL, $CVSSUPPORT, $CHATSUPPORT;
    global $CHECKLOGIN_WIKINAME;
    global $THISHOMEBASE;
    $firstinitstate = TBGetFirstInitState();

    #
    # The document base cannot be a mix of secure and nonsecure.
    #
    
    # create the main menu table, which also happens to reside in a form
    # (for search.)

    #
    # get post time of most recent news;
    # get both displayable version and age in days.
    #
    $query_result = 
	DBQueryFatal("SELECT DATE_FORMAT(date, '%M&nbsp;%e') AS prettydate, ".
		     " (TO_DAYS(NOW()) - TO_DAYS(date)) AS age ".
		     "FROM webnews ".
		     "WHERE archived=0 ".
		     "ORDER BY date DESC ".
		     "LIMIT 1");
    $newsDate = "";
    $newNews  = 0;

    #
    # This is so an admin can use the editing features of news.
    #
    if ($login_uid) { # && ISADMIN($login_uid)) { 
	$newsBase = $TBBASE; 
    } else {
	$newsBase = $TBDOCBASE;
    }

    if ($row = mysql_fetch_array($query_result)) {
	$newsDate = "(".$row[prettydate].")";
	if ($row[age] < 7) {
	    $newNews = 1;
	}
    }

    echo "<FORM method=get ACTION=$newsBase/search.php3>\n";
?>
  <script type='text/javascript' language='javascript' src='textbox.js'></script>
  <table class="menu" width=210 cellpadding="0" cellspacing="0">
    <tr><td class="menuheader"><b>Information</b></td></tr>
<?php
    if (0 == strcasecmp($THISHOMEBASE, "emulab.net")) {
	$rootEmulab = 1;
    } else {
	$rootEmulab = 0;
    }

    WRITESIDEBARBUTTON("Home", $TBDOCBASE, "index.php3?stayhome=1");


    if ($rootEmulab) {
	WRITESIDEBARBUTTON("Other Emulabs", $TBDOCBASE,
			       "docwrapper.php3?docname=otheremulabs.html");
    } else {
	WRITESIDEBARBUTTON_ABS("Utah Emulab", $TBDOCBASE,
			       "http://www.emulab.net/");

    }

    if ($newNews) {
	WRITESIDEBARBUTTON_NEW("News $newsDate", $newsBase, "news.php3");
    } else {
	WRITESIDEBARBUTTON("News $newsDate", $newsBase, "news.php3");
    }

    WRITESIDEBARBUTTON("Documentation", $TBDOCBASE, "doc.php3");

    if ($rootEmulab) {
	# Leave _NEW here about 2 weeks
	WRITESIDEBARBUTTON("Papers (Dec 24)", $TBDOCBASE, "pubs.php3");
	WRITESIDEBARBUTTON("Software (Jul 18)",
			       $TBDOCBASE, "software.php3");
	#WRITESIDEBARBUTTON("Add Widearea Node (CD)",
	#		    $TBDOCBASE, "cdrom.php");

	SIDEBARCELL("<a href=\"$TBDOCBASE/people.php3\">People</a> and " .
	            "<a href=\"$TBDOCBASE/gallery/gallery.php3\">Photos</a>");

	SIDEBARCELL("Emulab <a href=\"$TBDOCBASE/doc/docwrapper.php3? " .
		    "docname=users.html\">Users</a> and " .
	            "<a href=\"$TBDOCBASE/docwrapper.php3? " .
		    "docname=sponsors.html\">Sponsors</a>",1);
    } else {
	# Link ALWAYS TO UTAH
	#WRITESIDEBARBUTTON_ABSCOOL("Add Widearea Node (CD)",
	#		       $TBDOCBASE, "http://www.emulab.net/cdrom.php");
	WRITESIDEBARLASTBUTTON("Projects on Emulab", $TBDOCBASE,
			       "projectlist.php3");
    }

    # The actual search box. Form starts above ...
    echo "<tr><td class=menuoptst>
                 <input class='textInputEmpty' name=query
                        value='Search String'
                        size=16 onfocus='focus_text(this, \"Search String\")'
                        onblur='blur_text(this, \"Search String\")' />".
	"<input type=submit style='font-size:10px;' value=Search /><br>".
	"</td></tr>\n";
    WRITESIDEBARDIVIDER();

    #
    # Cons up a nice message.
    # 
    switch ($login_status & CHECKLOGIN_STATUSMASK) {
    case CHECKLOGIN_LOGGEDIN:
	$login_message = 0;
	    
	if ($login_status & CHECKLOGIN_PSWDEXPIRED)
	    $login_message = "$login_message<br>(Password Expired!)";
	elseif ($login_status & CHECKLOGIN_UNAPPROVED)
	    $login_message = "$login_message<br>(Unapproved!)";
	break;
    case CHECKLOGIN_TIMEDOUT:
	$login_message = "Login Timed out.";
	break;
    default:
	$login_message = 0;
	break;
    }

    if ($login_message) {
      echo "<tr>";
      echo "<td class=menuoptst style='padding-top: 6px;' ><center><b>";
      echo "$login_message</b></center></td>";
      echo "</tr>";
    }

    #
    # Now the login box. Remember, already inside a table.
    # We want the links to the login pages to always be https,
    # but the images path depends on whether the page was loaded as
    # http or https, since we do not want to mix them, since they
    # cause warnings.
    #
    if (! ($login_status & (CHECKLOGIN_LOGGEDIN|CHECKLOGIN_MAYBEVALID)) &&
	!NOLOGINS()) {
	echo "<tr>";
	echo "<td class=\"menuoptst\" align=center valign=center>";

	if (!$firstinitstate) {
	    echo "<a href=\"$TBBASE/reqaccount.php3\">";
	    echo "<img alt=\"Request Account\" border=0 ";
	    echo "src=\"$BASEPATH/requestaccount.gif\"></a>";

	    echo "<br /><b>or</b><br />";
	}

	echo "<a href=\"$TBBASE/login.php3\">";
	echo "<img alt=\"logon\" border=0 ";
	echo "src=\"$BASEPATH/logon.gif\"></a>\n";

	echo "</td></tr>\n";
    }

    #
    # Login message. Set via 'web/message' site variable
    #
    $message = TBGetSiteVar("web/message");
    if (0 != strcmp($message,"")) {
	WRITESIDEBARNOTICE($message);    	
    }

    echo "</table>\n";

    # Start Interaction section if going to spit out interaction options.
    if ($login_status & (CHECKLOGIN_LOGGEDIN|CHECKLOGIN_MAYBEVALID)) {
	echo "<table class=menu width=210 cellpadding=0 cellspacing=0>".
	    "<tr><td class=menuheader>".
	    "<b>Experimentation</b>".
	    "</td></tr>\n";
    }

    #
    # Basically, we want to let admin people continue to use
    # the web interface even when nologins set. But, we want to make
    # it clear its disabled.
    # 
    if (NOLOGINS()) {
	if (! ($login_status &
	       (CHECKLOGIN_LOGGEDIN|CHECKLOGIN_MAYBEVALID))) {
	    echo "<table class=menu width=210 cellpadding=0 cellspacing=0>".
		"<tr><td class=menuheader> &nbsp".
		"</td></tr>\n";
	}
	
        WRITESIDEBARBUTTON("<font color=red> ".
			   "Web Interface Temporarily Unavailable</font>",
			   $TBDOCBASE, "nologins.php3");

        if (!$login_uid || !ISADMIN($login_uid)) {	
	    WRITESIDEBARNOTICE("Please Try Again Later");
        }
	if (! ($login_status &
	       (CHECKLOGIN_LOGGEDIN|CHECKLOGIN_MAYBEVALID))) {
	    echo "</table>\n";
	}
    }

    if ($login_status & (CHECKLOGIN_LOGGEDIN|CHECKLOGIN_MAYBEVALID)) {
	if ($firstinitstate != null) {    
	    if ($firstinitstate == "createproject") {
		WRITESIDEBARBUTTON("Create First Project",
				   $TBBASE, "newproject.php3");
	    }
	    elseif ($firstinitstate == "approveproject") {
		$firstinitpid = TBGetFirstInitPid();
		
		WRITESIDEBARBUTTON("Approve First Project",
				   $TBBASE,
				   "approveproject.php3?pid=$firstinitpid".
				   "&approval=approve");
	    }
	}
	elseif ($login_status & CHECKLOGIN_ACTIVE) {
	    if ($login_status & CHECKLOGIN_PSWDEXPIRED) {
		WRITESIDEBARBUTTON("Change Your Password",
				   $TBBASE, "moduserinfo.php3");
	    }
	    elseif ($login_status & (CHECKLOGIN_WEBONLY|CHECKLOGIN_WIKIONLY)) {
		WRITESIDEBARBUTTON("My Emulab",
				   $TBBASE,
				   "showuser.php3?target_uid=$login_uid");

		if ($WIKISUPPORT && $CHECKLOGIN_WIKINAME != "") {
		    $wikiname = $CHECKLOGIN_WIKINAME;
		
		    WRITESIDEBARBUTTON_ABSCOOL("My Wikis",
			       "gotowiki.php3?redurl=Main/$wikiname",
			       "gotowiki.php3?redurl=Main/$wikiname");
		}

		WRITESIDEBARBUTTON("Update User Information",
				   $TBBASE, "moduserinfo.php3");
	    }
	    else {
		WRITESIDEBARBUTTON("My Emulab",
				   $TBBASE,
				   "showuser.php3?target_uid=$login_uid");

		#
                # Since a user can be a member of more than one project,
                # display this option, and let the form decide if the 
                # user is allowed to do this.
                #
 		WRITESIDEBARBUTTON("Begin an Experiment",
				   $TBBASE, "beginexp_html.php3");
	
		# Put _NEW back when Plab is working again.
		WRITESIDEBARBUTTON("Create a PlanetLab Slice",
				       $TBBASE, "plab_ez.php3");

		WRITESIDEBARBUTTON("Experiment List",
				   $TBBASE, "showexp_list.php3");

		WRITESIDEBARDIVIDER();

		WRITESIDEBARBUTTON("Node Status",
				   $TBBASE, "nodecontrol_list.php3");

		SIDEBARCELL("List <a " .
			"href=\"$TBBASE/showimageid_list.php3\">" .
	        	"ImageIDs</a> or <a " .
	                "href=\"$TBBASE/showosid_list.php3\">OSIDs</a>");

		if ($login_status & CHECKLOGIN_TRUSTED) {
		  WRITESIDEBARDIVIDER();
                  # Only project/group leaders can do these options
                  # Show a "new" icon if there are people waiting for approval
		  $query_result =
		    DBQueryFatal("select g.* from group_membership as authed ".
				 "left join group_membership as g on ".
				 " g.pid=authed.pid and g.gid=authed.gid ".
				 "left join users as u on u.uid=g.uid ".
				 "where u.status!='".
				 TBDB_USERSTATUS_UNVERIFIED . "' and ".
				 " u.status!='" . TBDB_USERSTATUS_NEWUSER . 
				 "' and g.uid!='$login_uid' and ".
				 "  g.trust='". TBDB_TRUSTSTRING_NONE . "' ".
				 "  and authed.uid='$login_uid' and ".
				 "  (authed.trust='group_root' or ".
				 "   authed.trust='project_root') ".
				 "ORDER BY g.uid,g.pid,g.gid");
		  if (mysql_num_rows($query_result) > 0) {
		    WRITESIDEBARBUTTON_NEW("New User Approval",
					   $TBBASE, "approveuser_form.php3");
		  } else {

		      WRITESIDEBARBUTTON("New User Approval",
				       $TBBASE, "approveuser_form.php3");
		  }
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
	#
	# Standard options for logged in users!
	#
	if (!$firstinitstate) {
	    WRITESIDEBARDIVIDER();
	    SIDEBARCELL("<a href=\"$TBBASE/newproject.php3\">Start</a> or " .
	             "<a href=\"$TBBASE/joinproject.php3\">Join</a> a Project",
			0);
	}
    }
    #WRITESIDEBARLASTBUTTON_COOL("Take our Survey",
    #    $TBDOCBASE, "survey.php3");

    # Terminate Interaction menu.
    if ($login_status & (CHECKLOGIN_LOGGEDIN|CHECKLOGIN_MAYBEVALID)) {
        # Logout option. No longer take up space with an image.
	WRITESIDEBARLASTBUTTON("<b>Logout</b>",
			       $TBBASE, "logout.php3?target_uid=$login_uid");
	
	echo "</table>\n";
    }

    # And now the Collaboration menu.
    if (($login_status & (CHECKLOGIN_LOGGEDIN|CHECKLOGIN_MAYBEVALID)) &&
	($WIKISUPPORT || $MAILMANSUPPORT || $BUGDBSUPPORT ||
	 $CVSSUPPORT  || $CHATSUPPORT)) {
	echo "<table class=menu width=210 cellpadding=0 cellspacing=0>".
	    "<tr><td class=menuheader>".
	    "<b>Collaboration</b>".
	    "</td></tr>\n";
     
	if ($WIKISUPPORT && $CHECKLOGIN_WIKINAME != "") {
	    $wikiname = $CHECKLOGIN_WIKINAME;
		
	    WRITESIDEBARBUTTON("My Wikis", $TBBASE,
			       "gotowiki.php3?redurl=Main/$wikiname");
	}
	if ($MAILMANSUPPORT || $BUGDBSUPPORT) {
	    if (!isset($pid) || $pid == "") {
		$query_result =
		    DBQueryFatal("select pid from group_membership where ".
				 "uid='$login_uid' and pid=gid and ".
				 "trust!='none' ".
				 "order by date_approved asc limit 1");
		if (mysql_num_rows($query_result)) {
		    $row = mysql_fetch_array($query_result);
		    $firstpid = $row[pid];
		}
	    }
	}
	if ($MAILMANSUPPORT) {
	    $mmurl  = "showmmlists.php3?target_uid=$login_uid";
	    WRITESIDEBARBUTTON("My Mailing Lists", $TBBASE, $mmurl);
	}
	if ($BUGDBSUPPORT) {
	    $bugdburl = "gotobugdb.php3";
		    
	    if (isset($pid) && !empty($pid)) {
		$bugdburl .= "?project_title=$pid";
	    }
	    elseif (isset($firstpid)) {
		$bugdburl .= "?project_title=$firstpid";
	    }
	    WRITESIDEBARBUTTON("My Bug Databases", $TBBASE, $bugdburl);
	}
	if ($CVSSUPPORT) {
	    WRITESIDEBARBUTTON("My CVS Repositories", $TBBASE,
			       "listrepos.php3?target_uid=$login_uid");
	}
	if ($CHATSUPPORT) {
	    WRITESIDEBARBUTTON("My Chat Buddies", $TBBASE,
			       "mychat.php3?target_uid=$login_uid");
	}
	WRITESIDEBARDIVIDER();
	echo "</table>\n";
    }

    # Optional ADMIN menu.
    if ($login_status & CHECKLOGIN_LOGGEDIN && ISADMIN($login_uid)) {
	echo "<table class=menu width=210 cellpadding=0 cellspacing=0>".
	    "<tr><td class=menuheader>".
	    "<b>Administration</b>".
	    "</td></tr>\n";
	
	SIDEBARCELL("List <a " .
		    " href=\"$TBBASE/showproject_list.php3\">" .
		    "Projects</a> or <a " .
		    "href=\"$TBBASE/showuser_list.php3\">Users</a>");

	WRITESIDEBARBUTTON("View Testbed Stats",
			   $TBBASE, "showstats.php3");

	WRITESIDEBARBUTTON("Approve New Projects",
			   $TBBASE, "approveproject_list.php3");

	WRITESIDEBARBUTTON("Edit Site Variables",
			   $TBBASE, "editsitevars.php3");

	WRITESIDEBARBUTTON("Edit Knowledge Base",
			   $TBBASE, "kb-manage.php3");
		    
	$query_result = DBQUeryFatal("select new_node_id from new_nodes");
	if (mysql_num_rows($query_result) > 0) {
	    WRITESIDEBARBUTTON_NEW("Add Testbed Nodes",
				   $TBBASE, "newnodes_list.php3");
	}
	else {
	    WRITESIDEBARBUTTON("Add Testbed Nodes",
			       $TBBASE, "newnodes_list.php3");
	}
	WRITESIDEBARBUTTON("Approve Widearea User",
			   $TBBASE, "approvewauser_form.php3");

	# Link ALWAYS TO UTAH
	WRITESIDEBARLASTBUTTON("Add Widearea Node (CD)",
			       $TBDOCBASE, "http://www.emulab.net/cdrom.php");
	echo "</table>\n";
    }
    echo "</form>\n";
}

#
# Simple version of above, that just writes the given menu.
# 
function WRITESIMPLESIDEBAR($menudefs) {
    $menutitle = $menudefs['title'];
    
    echo "<table class=menu width=210 cellpadding=0 cellspacing=0>
           <tr><td class=menuheader>\n";

    echo "<b>$menutitle</b>";
    echo " </td>";
    echo "</tr>\n";

    each($menudefs);    
    while (list($key, $val) = each($menudefs)) {
	WRITESIDEBARBUTTON("$key", null, "$val");
    }
    echo "</table>\n";
}

#
# spits out beginning part of page
#
function PAGEBEGINNING( $title, $nobanner = 0, $nocontent = 0,
        $extra_headers = NULL ) {
    global $BASEPATH, $TBMAINSITE, $THISHOMEBASE, $ELABINELAB;
    global $TBDIR, $WWW;
    global $MAINPAGE;
    global $TBDOCBASE;
    global $autorefresh;

    $MAINPAGE = !strcmp($TBDIR, "/usr/testbed/");

    echo "<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.01 Transitional//EN' 
          'http://www.w3.org/TR/html4/loose.dtd'>
	<html>
	  <head>
	    <title>$THISHOMEBASE - $title</title>
            <!--<link rel=\"SHORTCUT ICON\" HREF=\"netbed.ico\">-->
            <link rel=\"SHORTCUT ICON\" HREF=\"netbed.png\" TYPE=\"image/png\">
    	    <!-- dumbed-down style sheet for any browser that groks (eg NS47). -->
	    <link REL='stylesheet' HREF='$BASEPATH/common-style.css' TYPE='text/css' />
    	    <!-- do not import full style sheet into NS47, since it does bad job
            of handling it. NS47 does not understand '@import'. -->
    	    <style type='text/css' media='all'>
            <!-- @import '$BASEPATH/style.css'; -->";

    if (!$MAINPAGE) {
	echo "<!-- @import '$BASEPATH/style-nonmain.css'; -->";
    } 

    echo "</style>\n";

    if ($TBMAINSITE) {
	echo "<meta NAME=\"keywords\" ".
	           "CONTENT=\"network, emulation, internet, emulator, ".
	           "mobile, wireless, robotic\">\n";
	echo "<meta NAME=\"robots\" ".
	           "CONTENT=\"NOARCHIVE\">\n";
	echo "<meta NAME=\"description\" ".
                   "CONTENT=\"emulab - network emulation testbed home\">\n";
    }
    if ($extra_headers) {
        echo $extra_headers;
    }
    echo "</head>
            <body bgcolor='#FFFFFF' 
             topmargin='0' leftmargin='0' marginheight='0' marginwidth='0'>\n";
    
    if ($autorefresh) {
	echo "<meta HTTP-EQUIV=\"Refresh\" CONTENT=\"$autorefresh\">\n";
    }
    if (! $nobanner ) {
	echo "<map name=overlaymap>
                 <area shape=rect coords=\"100,60,369,100\"
                       href='http://www.emulab.net/index.php3'>
                 <area shape=rect coords=\"0,0,369,100\"
                       href='$TBDOCBASE/index.php3'>
              </map>
            <table cellpadding='0' cellspacing='0' width='100%'>
            <tr valign='top'>
              <td valign='top' class='bannercell'
                  background='$BASEPATH/headerbgbb.jpg'
                  bgcolor=#3D627F>
              <img width=369 height=100 border=0 usemap=\"#overlaymap\" ";

	if ($ELABINELAB) {
	    echo "src='$BASEPATH/overlay.elabinelab.gif' ";
	}
	else {
	    echo "src='$BASEPATH/overlay.".strtolower($THISHOMEBASE).".gif' ";
	}
	echo "alt='$THISHOMEBASE - the network testbed'>\n";
        if (!$MAINPAGE) {
	     echo "<font size='+1' color='#CCFFCC'>&nbsp;<b>$WWW</b></font>";
	}
	echo "</td>\n";
	echo "<td valign=top align=right width=200
                 class=banneriframe>
             <iframe src=$BASEPATH/currentusage.php3
                 width=100% height=100 marginheight=0 marginwidth=0
                 scrolling=no frameborder=0></iframe></td>\n";
        echo "</tr></table>\n";
    }
    if (! $nocontent ) {
	echo "<table cellpadding='8' cellspacing='0' height='100%'
                width='100%'>
                <tr height='100%'>
                  <td valign='top' class='leftcell' bgcolor='#ccddee'>
                  <!-- sidebar begins -->";
    }
}

#
# finishes sidebar td
#
function FINISHSIDEBAR()
{
    echo "<!-- sidebar ends -->
        </td>
        <td valign='top' width='100%' class='rightcell'>
          <!-- content body table -->
          <table class='content' width='100%' cellpadding='0' cellspacing='0'>
            <tr>
              <td class='contentheader'>";
}

#
# Spit out a vanilla page header.
#
function PAGEHEADER($title, $view = NULL, $extra_headers = NULL) {
    global $login_status, $login_uid, $TBBASE, $TBDOCBASE, $THISHOMEBASE;
    global $BASEPATH, $SSL_PROTOCOL, $drewheader, $autorefresh;
    global $TBMAINSITE;

    $drewheader = 1;
    if (isset($_GET['refreshrate']) && is_numeric($_GET['refreshrate'])) {
	$autorefresh = $_GET['refreshrate'];
    }

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
    # If no view options were specified, get the ones for the current user
    #
    if (!$view) {
	$view = GETUSERVIEW();
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

    if (isset($view['hide_banner'])) {
	$nobanner = 1;
    } else {
	$nobanner = 0;
    }
    PAGEBEGINNING( $title, $nobanner, 0, $extra_headers );
    if (!isset($view['hide_sidebar'])) {
	WRITESIDEBAR();
    }
    elseif (isset($view['menu'])) {
	WRITESIMPLESIDEBAR($view['menu']);
    }
    FINISHSIDEBAR();
    echo "<h2 class=\"nomargin\">\n";

    if ($login_uid && ISADMINISTRATOR()) {
	if (ISADMIN($login_uid)) {
	    echo "<a href=$TBBASE/toggle.php?target_uid=$login_uid&type=adminoff&value=1>
	             <img src='/redball.gif'
                          border=0 alt='Admin On'></a>\n";
	}
	else {
	    echo "<a href=$TBBASE/toggle.php?target_uid=$login_uid&type=adminoff&value=0>
	             <img src='/greenball.gif'
                          border=0 alt='Admin Off'></a>\n";
	}
    }
    $major = "";
    $minor = "";
    $build = "";
    TBGetVersionInfo($major, $minor, $build);
    if ($view['hide_versioninfo'] == 1)
	$versioninfo = "";
    else
	$versioninfo = "Vers: $major.$minor Build: $build";
    
    $now = date("D M d g:ia T");
    echo "$title</h2></td>\n";
    echo "<td class=contentheader align=right>\n";
    echo "<table border='0' cellpadding='0' cellspacing='0'>";
    echo "  <tr>";
    echo "  <td class=contentheader><font size=-2>$versioninfo</font></td>";
    echo "  <td class=contentheader>&nbsp&nbsp</td>";
    echo "  <td class=contentheader align=right>";
    if ($login_uid) {
	echo "<font size=-1>'<b>$login_uid</b>' Logged in.<br>$now</font>\n";
    }
    else {
	echo "$now";
    }
    echo "</td>";
    echo "</tr>";
    echo "</table>";
    echo "</td>";
    echo "</tr>\n";
    echo "<tr><td colspan=3 class=\"contentbody\" width=*>";
    echo "<!-- begin content -->\n";
    if ($view['show_topbar'] == "plab") {
	WRITEPLABTOPBAR();
    }
}

#
# ENDPAGE(): This terminates the table started above.
# 
function ENDPAGE() {
  echo "</td></tr></table>";
  echo "</td></tr></table>";
}

#
# Spit out a vanilla page footer.
#
function PAGEFOOTER($view = NULL) {
    global $TBDOCBASE, $TBMAILADDR, $THISHOMEBASE;
    global $TBMAINSITE, $SSL_PROTOCOL;

    if (!$view) {
	$view = GETUSERVIEW();
    }

    $today = getdate();
    $year  = $today["year"];

    echo "<!-- end content -->\n";
    if ($view['show_bottombar'] == "plab") {
	WRITEPLABBOTTOMBAR();
    }

    echo "
              </td>
            </tr>
            <tr>
              <td colspan=2 class=contentbody>\n";
    if (!$view['hide_copyright']) {
	echo "
	        <center>
                <font size=-1>
		[ <a href=http://www.cs.utah.edu/flux/>
                    Flux&nbsp;Research&nbsp;Group</a> ]
		[ <a href=http://www.cs.utah.edu/>
                    School&nbsp;of&nbsp;Computing</a> ]
		[ <a href=http://www.utah.edu/>
                    University&nbsp;of&nbsp;Utah</a> ]
		</font>
		<br>
                <!-- begin copyright -->
                <font size=-2>
                <a href='$TBDOCBASE/docwrapper.php3?docname=copyright.html'>
                    Copyright &copy; 2000-$year The University of Utah</a>
                </font>
                <br>
		</center>\n";
    }
    echo "
                <p align=right>
		  <font size=-2>
                    Problems?
	            Contact $TBMAILADDR.
                  </font>
                </p>
                <!-- end copyright -->\n";

    ENDPAGE();

    # Plug the home site from all others.
    echo "\n<p><a href=\"www.emulab.net/netemu.php3\"></a>\n";

    echo "</body></html>\n";
}

function PAGEERROR($msg) {
    global $drewheader, $noheaders;

    if (! $drewheader && ! $noheaders)
	PAGEHEADER("Page Error");

    echo "$msg\n";

    if (! $noheaders) 
	PAGEFOOTER();
    die("");
}

#
# Sub Page/Menu Stuff
#
function WRITESUBMENUBUTTON($text, $link, $target = "") {

    #
    # Optional 'target' agument, so that we can pop up new windows
    #
    if ($target) {
	$targettext = "target='$target'";
    }

    echo "<!-- Table row for button $text -->
          <tr>
            <td class=submenuopt nowrap>
         	 <a href='$link' $targettext>$text</a>
            </td>
          </tr>\n";
}

function WRITESUBMENUDIVIDER() {
    global $BASEPATH;
    echo "<tr>";
    echo "<td class=\"submenuoptdiv\">";
    # We have to put something in this cell, or IE ignores it. But, we do not
    # want to make the table row full line-height, so a space will not do.
    echo "<img src=\"$BASEPATH/1px.gif\" border=0 height=1 width=1 />";
    echo "</td>";
    echo "</tr>";
    echo "\n";
}

#
# Start/End a page within a page. 
#
function SUBPAGESTART() {
    echo "<!-- begin subpage -->";
    echo "<table class=\"stealth\"
	  cellspacing='0' cellpadding='0' width='100%' border='0'>\n
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
    <table class="submenu" cellpadding="0" cellspacing="0">
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
  <td class="stealth" valign=top align=left width='100%'>
<?php
}

# Start a new section in an existing submenu
# This includes ending the one before it
function SUBMENUSECTION($title) {
    SUBMENUSECTIONEND();
?>
      <!-- new submenu section -->
      <tr>
        <td class="menuheader"><b><?php echo "$title";?></b></td>
      </tr>
<?php
}

# End a submenu section - only need this on the last one of the table.
function SUBMENUSECTIONEND() {
?>
      <tr height=5><td></td></tr>
<?php
}

# These are here so you can wedge something else under the menu in the left column.

function SUBMENUEND_2A() {
?>
    </table>
    <!-- end submenu -->
<?php
}

function SUBMENUEND_2B() {
?>
  </td>
  <td class="stealth" valign=top align=left width='85%'>
<?php
}

#
# Get a view, for use with PAGEHEADER and PAGEFOOTER, for the current user
#
function GETUSERVIEW() {
    if (GETUID() && ISPLABUSER()) {
	return array('hide_sidebar' => 1, 'hide_banner' => 1,
	    'show_topbar' => "plab", 'show_bottombar' => 'plab',
	    'hide_copyright' => 1);
    } else {
	# Most users get the default view
	return array();
    }
}

?>
