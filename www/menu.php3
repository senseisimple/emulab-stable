<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#

$login_user       = null;
$login_status     = CHECKLOGIN_NOTLOGGEDIN;
$drewheader       = 0;
$noheaders	  = 0;
$autorefresh      = 0;
$currentusage     = 1;
$currently_busy   = 0;
$bodyclosestring  = "";

#
# This has to be set so we can spit out http or https paths properly!
# Thats because browsers do not like a mix of secure and nonsecure.
# 
$BASEPATH	  = "";

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

# Blank space dividers in the menus are handled by adding a class to the menu
# item that starts a new group.
$nextsidebarcl    = "";
$nextsubmenucl    = "";

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
    global $nextsidebarcl;
    
    $nextsidebarcl = "newgroup";
}

#
# WRITESIDEBARBUTTON(text, link): Write a button on the sidebar menu.
# We do not currently try to match the current selection so that its
# link looks different. Not sure its really necessary.
#
function WRITESIDEBARBUTTON($text, $base, $link, $extratext="") {
    global $nextsidebarcl;
    
    if ($base)
	$link = "$base/$link";
    $cl = "";
    if ($nextsidebarcl != "") {
	$cl = "class='$nextsidebarcl'";
	$nextsidebarcl = "";
    }
    echo "<li $cl><a href=\"$link\">$text</a>$extratext</li>\n";
}

# same as above with "new" gif next to it.
function WRITESIDEBARBUTTON_NEW($text, $base, $link) {
    WRITESIDEBARBUTTON($text,
		       $base,
		       $link,
		       "&nbsp;<img src=\"/new.gif\" />");
}

# same as above with "cool" gif next to it.
function WRITESIDEBARBUTTON_COOL($text, $base, $link ) {
    $link = "$base/$link";
    echo "<li><a href=\"$link\">$text</a>&nbsp;<img src=\"/cool.gif\" /></li>\n";
}

function WRITESIDEBARBUTTON_ABS($text, $base, $link ) {
    $link = "$link";
    echo "<li><a href=\"$link\">$text</a></li>\n";
}

function WRITESIDEBARBUTTON_ABSCOOL($text, $base, $link ) {
    $link = "$link";
    echo "<li><a href=\"$link\">$text</a>&nbsp;<img src=\"/cool.gif\" /></li>\n";
}

# writes a message to the sidebar, without clickability.
function WRITESIDEBARNOTICE($text) {
    echo "<span class='notice'>$text</span>\n";
}

#
# Something like the sidebar, but across the top, with only a few options.
# Think Google. For PlanetLab users, but it would be easy enough to make
# others. Still a work in progress.
#
function WRITEPLABTOPBAR() {
    echo "<table class=\"topbar\" width=\"100%\" cellpadding=\"2\" cellspacing=\"0\" align=\"center\">\n";
    global $login_status, $login_user;
    global $TBBASE, $TBDOCBASE, $BASEPATH;
    global $THISHOMEBASE;

    WRITETOPBARBUTTON("Create a Slice",
        $TBBASE, "plab_ez.php3");

    WRITETOPBARBUTTON("Nodes",
        $TBBASE, "plabmetrics.php3");

    WRITETOPBARBUTTON("My Testbed",
		      $TBBASE, CreateURL("showuser", $login_user));

    WRITETOPBARBUTTON("Advanced Experiment",
        $TBBASE, "beginexp_html.php3");

    if ($login_status & CHECKLOGIN_TRUSTED && $login_user->ApprovalList(0)) {
	WRITESIDEBARBUTTON_NEW("Approve Users",
			       $TBBASE, "approveuser_form.php3");
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
    global $login_status, $login_user;
    global $TBBASE, $TBDOCBASE, $BASEPATH;
    global $THISHOMEBASE;

    if ($login_user) {
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
    global $login_status, $login_user, $pid, $gid;
    global $TBBASE, $TBDOCBASE, $BASEPATH, $WIKISUPPORT, $MAILMANSUPPORT;
    global $BUGDBSUPPORT, $BUGDBURL, $CVSSUPPORT, $CHATSUPPORT;
    global $CHECKLOGIN_WIKINAME;
    global $THISHOMEBASE;
    global $EXPOSETEMPLATES;
    $firstinitstate = TBGetFirstInitState();

    #
    # The document base cannot be a mix of secure and nonsecure.
    #
    
    # create the main menu list

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
    if ($login_user) {
	$newsBase = $TBBASE; 
    } else {
	$newsBase = $TBDOCBASE;
    }

    if ($row = mysql_fetch_array($query_result)) {
	$newsDate = "(".$row["prettydate"].")";
	if ($row["age"] < 7) {
	    $newNews = 1;
	}
    }

    echo "<script type='text/javascript' language='javascript'
                  src='${BASEPATH}/textbox.js'></script>\n";
    echo "<h3 class=menuheader id=informationheader>Information</h3><ul class=menu id=information>\n";

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
	WRITESIDEBARBUTTON_NEW("Papers and Talks (Mar 9)", $TBDOCBASE, "pubs.php3");
	WRITESIDEBARBUTTON("Software (Jul 18)",
			       $TBDOCBASE, "software.php3");
	#WRITESIDEBARBUTTON("Add Widearea Node (CD)",
	#		    $TBDOCBASE, "cdrom.php");

	echo "<li><a href=\"$TBDOCBASE/people.php3\">People</a> and " .
	     "<a href=\"$TBDOCBASE/gallery/gallery.php3\">Photos</a>" .
	     "</li>";
# with New!
#	echo "<li><a href=\"$TBDOCBASE/people.php3\">People</a> and " .
#	     "<a href=\"$TBDOCBASE/gallery/gallery.php3\">Photos</a>" .
#	     "&nbsp;<img src=\"/new.gif\" />" .
#	     "</li>";

	echo "<li>Emulab <a href=\"$TBDOCBASE/doc/docwrapper.php3? " .
	     "docname=users.html\">Users</a> and " .
	     "<a href=\"$TBDOCBASE/docwrapper.php3? " .
	     "docname=sponsors.html\">Sponsors</a></li>";
    } else {
	# Link ALWAYS TO UTAH
	#WRITESIDEBARBUTTON_ABSCOOL("Add Widearea Node (CD)",
	#		       $TBDOCBASE, "http://www.emulab.net/cdrom.php");
	WRITESIDEBARBUTTON("Projects on Emulab", $TBDOCBASE,
		"projectlist.php3");
    }
    
    echo "</ul>\n";

    # The search box.  Placed in a table so the text input fills available
    # space.
    echo "<div id='searchrow'>
        <form method='get' action='$newsBase/search.php3'>
        <table border='0' cellspacing='0' cellpadding='0'><tr>
             <td width='100%'><input class='textInputEmpty' name='query'
                        value='Search String' id='searchbox'
                        onfocus='focus_text(this, \"Search String\")'
                        onblur='blur_text(this, \"Search String\")' /></td>
	     <td><input type='submit' id='searchsub' value=Search /></td>
        </table>
        </form>
	</div>\n";

    #
    # Cons up a nice message.
    # 
    switch ($login_status & CHECKLOGIN_STATUSMASK) {
    case CHECKLOGIN_LOGGEDIN:
	if ($login_status & CHECKLOGIN_PSWDEXPIRED)
	    $login_message = "(Password Expired!)";
	elseif ($login_status & CHECKLOGIN_UNAPPROVED)
	    $login_message = "(Unapproved!)";
	else
	    $login_message = 0;
	break;
    case CHECKLOGIN_TIMEDOUT:
	$login_message = "Login Timed out.";
	break;
    default:
	$login_message = 0;
	break;
    }

    #
    # Now the login box.
    # We want the links to the login pages to always be https,
    # but the images path depends on whether the page was loaded as
    # http or https, since we do not want to mix them, since they
    # cause warnings.
    #
    if (! ($login_status & (CHECKLOGIN_LOGGEDIN|CHECKLOGIN_MAYBEVALID)) &&
	!NOLOGINS()) {
	echo "<div id='loginbox'>";

	if ($login_message) {
	    echo "<strong>$login_message</strong>";
	}

	if (!$firstinitstate) {
	    echo "<a href=\"$TBBASE/reqaccount.php3\">";
	    echo "<img alt=\"Request Account\" border=0 ";
	    echo "src=\"$BASEPATH/requestaccount.gif\" width=\"144\" height=\"32\"></a>";

	    echo "<strong>or</strong>";
	}

	echo "<a href=\"$TBBASE/login.php3\">";
	echo "<img alt=\"logon\" border=0 ";
	echo "src=\"$BASEPATH/logon.gif\" width=\"144\" height=\"32\"></a>\n";

	echo "</div>";
    }

    #
    # Login message. Set via 'web/message' site variable
    #
    $message = TBGetSiteVar("web/message");
    if (0 != strcmp($message,"")) {
	WRITESIDEBARNOTICE($message);
    }

    #
    # Basically, we want to let admin people continue to use
    # the web interface even when nologins set. But, we want to make
    # it clear its disabled.
    # 
    if (NOLOGINS()) {
	echo "<a id='webdisabled' href='$TBDOCBASE/nologins.php3'>".
	    "Web Interface Temporarily Unavailable</a>";

        if (!$login_user || !ISADMIN()) {	
	    WRITESIDEBARNOTICE("Please Try Again Later");
        }
    }

    # Start Interaction section if going to spit out interaction options.
    if ($login_status & (CHECKLOGIN_LOGGEDIN|CHECKLOGIN_MAYBEVALID)) {
	echo "<h3 class='menuheader' id='experimentationheader'>Experimentation</h3>
              <ul class='menu' id='experimentation'>\n";
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
				   CreateURL("showuser", $login_user));
					     
		if ($WIKISUPPORT && $CHECKLOGIN_WIKINAME != "") {
		    $wikiname = $CHECKLOGIN_WIKINAME;
		
		    WRITESIDEBARBUTTON_ABSCOOL("My Wikis",
			       "gotowiki.php3?redurl=Main/$wikiname",
			       "gotowiki.php3?redurl=Main/$wikiname");
		}

		WRITESIDEBARBUTTON("Update User Information",
				   $TBBASE,
				   CreateURL("moduserinfo", $login_user));
	    }
	    else {
		WRITESIDEBARBUTTON("My Emulab",
				   $TBBASE,
				   CreateURL("showuser", $login_user));

		#
                # Since a user can be a member of more than one project,
                # display this option, and let the form decide if the 
                # user is allowed to do this.
                #
 		WRITESIDEBARBUTTON("Begin an Experiment",
				   $TBBASE, "beginexp_html.php3");

		if ($EXPOSETEMPLATES) {
		    WRITESIDEBARBUTTON("Create a Template",
				       $TBBASE, "template_create.php");
		}
	
		# Put _NEW back when Plab is working again.
		WRITESIDEBARBUTTON("Create a PlanetLab Slice",
				       $TBBASE, "plab_ez.php3");

		WRITESIDEBARBUTTON("Experiment List",
				   $TBBASE, "showexp_list.php3");

		WRITESIDEBARDIVIDER();
		
		WRITESIDEBARBUTTON("Node Status",
				   $TBBASE, "nodecontrol_list.php3");

		echo "<li>List <a " .
			"href=\"$TBBASE/showimageid_list.php3\">" .
	        	"ImageIDs</a> or <a " .
	                "href=\"$TBBASE/showosid_list.php3\">OSIDs</a></li>";

		if ($login_status & CHECKLOGIN_TRUSTED &&
		    $login_user->ApprovalList(0)) {
		    WRITESIDEBARDIVIDER();
		    WRITESIDEBARBUTTON_NEW("New User Approval",
					   $TBBASE, "approveuser_form.php3");
		}
	    }
	}
	elseif ($login_status & (CHECKLOGIN_UNVERIFIED|CHECKLOGIN_NEWUSER)) {
	    WRITESIDEBARBUTTON("New User Verification",
			       $TBBASE, "verifyusr_form.php3");
	    WRITESIDEBARBUTTON("Update User Information",
			       $TBBASE,
			       CreateURL("moduserinfo", $login_user));
	}
	elseif ($login_status & (CHECKLOGIN_UNAPPROVED)) {
	    WRITESIDEBARBUTTON("Update User Information",
			       $TBBASE,
			       CreateURL("moduserinfo", $login_user));
			       
	}
	#
	# Standard options for logged in users!
	#
	if (!$firstinitstate) {
	    echo "<li class='newgroup'><a href=\"$TBBASE/newproject.php3\">Start</a> or " .
		"<a href=\"$TBBASE/joinproject.php3\">Join</a> a Project</li>";
	}
    }
    #WRITESIDEBARLASTBUTTON_COOL("Take our Survey",
    #    $TBDOCBASE, "survey.php3");

    # Terminate Interaction menu.
    if ($login_status & (CHECKLOGIN_LOGGEDIN|CHECKLOGIN_MAYBEVALID)) {
        # Logout option. No longer take up space with an image.
	WRITESIDEBARBUTTON("<b>Logout</b>",
			   $TBBASE,
			   CreateURL("logout", $login_user));
	echo "</ul>\n";
    }

    # And now the Collaboration menu.
    if (($login_status & (CHECKLOGIN_LOGGEDIN|CHECKLOGIN_MAYBEVALID)) &&
	($WIKISUPPORT || $MAILMANSUPPORT || $BUGDBSUPPORT ||
	 $CVSSUPPORT  || $CHATSUPPORT)) {
	echo "<h3 class='menuheader' id='collaborationheader'>Collaboration</h3>
              <ul class='menu' id='collaboration'>";

	if ($WIKISUPPORT && $CHECKLOGIN_WIKINAME != "") {
	    $wikiname = $CHECKLOGIN_WIKINAME;
		
	    WRITESIDEBARBUTTON("My Wikis", $TBBASE,
			       "gotowiki.php3?redurl=Main/$wikiname");
	}
	if ($MAILMANSUPPORT || $BUGDBSUPPORT) {
	    if (!isset($pid) || $pid == "") {
		if (($project = $login_user->FirstApprovedProject())) {
		    $firstpid = $project->pid();
		}
	    }
	}
	if ($MAILMANSUPPORT) {
	     WRITESIDEBARBUTTON("My Mailing Lists", $TBBASE,
				CreateURL("showmmlists", $login_user));
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
			       CreateURL("listrepos", $login_user));
	}
	if ($CHATSUPPORT) {
	    WRITESIDEBARBUTTON("My Chat Buddies", $TBBASE,
			       CreateURL("mychat", $login_user));
	}
	echo "</ul>\n";
    }

    # Optional ADMIN menu.
    if ($login_status & CHECKLOGIN_LOGGEDIN && ISADMIN()) {
	echo "<h3 class='menuheader' id='administrationheader'>Administration</h3>
              <ul class='menu' id='administration'>";
	
	echo "<li>List <a " .
	    " href=\"$TBBASE/showproject_list.php3\">" .
	    "Projects</a> or <a " .
	    "href=\"$TBBASE/showuser_list.php3\">Users</a></li>";

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
	WRITESIDEBARBUTTON("Add Widearea Node (CD)",
			   $TBDOCBASE, "http://www.emulab.net/cdrom.php");
	echo "</ul>\n";
    }
}

#
# Simple version of above, that just writes the given menu.
# 
function WRITESIMPLESIDEBAR($menudefs) {
    $menutitle = $menudefs['title'];
    
    echo "<h3 class='menuheader'>$menutitle</h3>
          <ul class='menu'>";

    each($menudefs);    
    while (list($key, $val) = each($menudefs)) {
	WRITESIDEBARBUTTON("$key", null, "$val");
    }
    echo "</ul>\n";
}

#
# spits out beginning part of page
#
function PAGEBEGINNING( $title, $nobanner = 0, $nocontent = 0,
        $extra_headers = NULL ) {
    global $BASEPATH, $TBMAINSITE, $THISHOMEBASE, $ELABINELAB, $FANCYBANNER;
    global $TBDIR, $WWW;
    global $MAINPAGE;
    global $TBDOCBASE;
    global $autorefresh, $currentusage;

    $MAINPAGE = !strcmp($TBDIR, "/usr/testbed/");

    echo "<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.01 Transitional//EN' 
          'http://www.w3.org/TR/html4/loose.dtd'>
	<html>
	  <head>
	    <title>$THISHOMEBASE - $title</title>
            <link rel=\"shortcut icon\" href=\"favicon.ico\" TYPE=\"image/vnd.microsoft.icon\">
    	    <!-- dumbed-down style sheet for any browser that groks (eg NS47). -->
	    <link rel='stylesheet' href='$BASEPATH/common-style.css' type='text/css' />
    	    <!-- do not import full style sheet into NS47, since it does bad job
            of handling it. NS47 does not understand '@import'. -->
    	    <style type='text/css' media='all'>
            <!-- @import url($BASEPATH/style.css); -->";

    if (!$MAINPAGE) {
	echo "<!-- @import url($BASEPATH/style-nonmain.css); -->";
    } 

    echo "</style>\n";

    if ($TBMAINSITE) {
	echo "<meta name=\"keywords\" ".
	           "content=\"network, emulation, internet, emulator, ".
	           "mobile, wireless, robotic\">\n";
	echo "<meta name=\"robots\" ".
	           "content=\"NOARCHIVE\">\n";
	echo "<meta name=\"description\" ".
                   "content=\"emulab - network emulation testbed home\">\n";
    }
    if ($extra_headers) {
        echo $extra_headers;
    }
    echo "</head>
            <body bgcolor='#FFFFFF' 
             topmargin='0' leftmargin='0' marginheight='0' marginwidth='0'>\n";
    
    if ($autorefresh) {
	echo "<meta HTTP-EQUIV=\"Refresh\" content=\"$autorefresh\">\n";
    }
    echo "<script type='text/javascript' language='javascript'
                  src='${BASEPATH}/emulab_sup.js'></script>\n";

    if (! $nobanner ) {
        #
        # We do the banner differently for the Utah site and other sites.
        # The process of generating the fancy Utah banner is kind of 
        # complicated
        #
        if ($FANCYBANNER) {
            echo "<div id='fancybannercell'>\n";
        } else {
            echo "<div id='bannercell'>\n";
        }

        # NOTE: This has to come before any images in the div for the float to
        # work correctly.
	if ($currentusage) {
            if ($FANCYBANNER) {
                $class = "transparentusageframe";
            } else {
                $class = "usageframe";
            }
	    echo "<iframe src='$BASEPATH/currentusage.php3' class='$class'
                          scrolling='no' frameborder='0'></iframe>\n";
	}

        if ($FANCYBANNER) {
            echo "<a href='$TBDOCBASE/index.php3'>
                  <img height='100px' width='365px' border='0' src='$BASEPATH/fancy-header.png' />
                  </a>\n";
        } else {
            echo "<map name='overlaymap'>
                     <area shape=\"rect\" coords=\"100,60,339,100\"
                           href='http://www.emulab.net/index.php3'>
                     <area shape=\"rect\" coords=\"0,0,339,100\"
                           href='$TBDOCBASE/index.php3'>
                  </map>\n";
                  echo "<img height='100' border='0' usemap=\"#overlaymap\" ";
            if ($ELABINELAB) {
                echo "width='239' ";
                echo "src='$BASEPATH/overlay.elabinelab.gif' ";
            }
            else {
                echo "width='339' ";
                echo "src='$BASEPATH/overlay.".strtolower($THISHOMEBASE).".gif' ";
                echo "src='$BASEPATH/banner-rgb.png' ";
            }
            echo "alt='$THISHOMEBASE - the network testbed'>\n";
            if (0 && !$MAINPAGE) {
                echo "<span class='devpagename'>$WWW</span>";
            }
        }
        echo "</div>\n";
    }
    if (! $nocontent ) {
	echo "<div class='leftcell'>
                  <!-- sidebar begins -->";
    }
}

#
# finishes sidebar td
#
function FINISHSIDEBAR($contentname = "content", $nocontent = 0)
{
    global $TBMAINSITE, $TBBASE, $BASEPATH, $login_user;

    if (!$nocontent) {
	if (!$TBMAINSITE) {
	    #
	    # It is a violation of Emulab licensing restrictions to remove
	    # this logo!
	    #
	    echo "       <a class='builtwith' href='http://www.emulab.net'>
                         <img src='$BASEPATH/builtwith.png'></a>";
	}
	elseif ($login_user) {
	    echo "<span class=gripe><a href='$TBBASE/gotobugdb.php3".
	                    "?do=newtask&project_title=Emulab'>";
	    echo "Report Bug, Gripe, Request Feature</a>";
	    echo "</span>\n";
	}
	echo "<!-- sidebar ends -->
              </div>";
    }

    echo "
        <div class='$contentname'>
          <!-- content body -->";
}

#
# Spit out a vanilla page header.
#
function PAGEHEADER($title, $view = NULL, $extra_headers = NULL) {
    global $login_status, $login_user;
    global $TBBASE, $TBDOCBASE, $THISHOMEBASE;
    global $BASEPATH, $SSL_PROTOCOL, $drewheader, $autorefresh;
    global $TBMAINSITE;

    $drewheader = 1;
    if (isset($_GET['refreshrate']) && is_numeric($_GET['refreshrate'])) {
	$autorefresh = $_GET['refreshrate'];
    }

    #
    # Figure out who is logged in, if anyone.
    #
    if (($login_user = CheckLogin($status)) != null) {
	$login_status = $status;
	$login_uid    = $login_user->uid();
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
    if (NOLOGINS() && $login_user && !ISADMIN()) {
	DOLOGOUT($login_user);
	$login_status = CHECKLOGIN_NOTLOGGEDIN;
	$login_user   = null;
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

    if (VIEWSET($view, 'hide_banner')) {
	$nobanner = 1;
    } else {
	$nobanner = 0;
    }
    $contentname = "content";
    $nocontent = VIEWSET($view, 'hide_sidebar') && !VIEWSET($view, 'menu');
    PAGEBEGINNING( $title, $nobanner,
		   $nocontent,
		   $extra_headers );
    if (!VIEWSET($view, 'hide_sidebar')) {
	WRITESIDEBAR();
    }
    elseif (VIEWSET($view, 'menu')) {
	WRITESIMPLESIDEBAR($view['menu']);
    }
    else {
	$contentname = "fullcontent";
    }
    FINISHSIDEBAR($contentname, $nocontent);

    echo "<div class='contentbody'>";
    echo "<div id='logintime'>";
    echo "<span id='loggedin'>";
    $now = date("D M d g:ia T");
    if ($login_user) {
	echo "<span class='uid'>$login_uid</span> Logged in.";
    }
    echo "</span>";
    echo "<span class='timestamp'>$now</span>\n";
    echo "</div>";
    
    $major = "";
    $minor = "";
    $build = "";
    TBGetVersionInfo($major, $minor, $build);
    if (VIEWSET($view, 'hide_versioninfo'))
	$versioninfo = "";
    else
	$versioninfo = "Vers: $major.$minor Build: $build";
    echo "<div id='versioninfo'>$versioninfo</div>";

    echo "<h2 class='contenttitle'>\n";
    if ($login_user && ISADMINISTRATOR()) {
	if (ISADMIN()) {
	    $url = CreateURL("toggle", $login_user,
			     "type", "adminon", "value", 0);

	    echo "<a href=\"$TBBASE/$url\">
                      <img src='/redball.gif'
                          border='0' alt='Admin On'></a>\n";
	}
	else {
	    $url = CreateURL("toggle", $login_user,
			     "type", "adminon", "value", 1);

	    echo "<a href=\"$TBBASE/$url\">
                       <img src='/greenball.gif'
                          border='0' alt='Admin Off'></a>\n";
	}
    }
    echo "$title</h2>\n";

    if ($login_user) {
	echo "<div class=pagenotworking>Page not working properly? ";
	echo "<a href=pagenotworking.php>Click here</a></div>";
    }

    echo "<div class='cbody'>\n";
    echo "<!-- begin content -->\n";
    if (VIEWSET($view, 'show_topbar', "plab")) {
	WRITEPLABTOPBAR();
    }
}

#
# ENDPAGE(): This terminates the div started above.
# 
function ENDPAGE() {
  echo "</div>";
}

#
# Spit out a vanilla page footer.
#
function PAGEFOOTER($view = NULL) {
    global $TBDOCBASE, $TBMAILADDR, $THISHOMEBASE, $BASEPATH, $TBBASE;
    global $TBMAINSITE, $SSL_PROTOCOL, $bodyclosestring, $currently_busy;
    global $login_user;

    if ($currently_busy) {
	CLEARBUSY();
	$currently_busy = 0;
    }

    if (!$view) {
	$view = GETUSERVIEW();
    }

    $today = getdate();
    $year  = $today["year"];

    echo "<!-- end content -->\n";
    echo "</div>\n";

    if (VIEWSET($view, 'show_bottombar', "plab")) {
	WRITEPLABBOTTOMBAR();
    }

    echo "
              <div class='contentfooter'>\n";
    if (!VIEWSET($view, 'hide_copyright')) {
	echo "
                <ul class='navlist'>
		<li>[&nbsp;<a href=\"http://www.cs.utah.edu/flux/\"
                    >Flux&nbsp;Research&nbsp;Group</a>&nbsp;]</li>
	        <li>[&nbsp;<a href=\"http://www.cs.utah.edu/\"
                    >School&nbsp;of&nbsp;Computing</a>&nbsp;]</li>
		<li>[&nbsp;<a href=\"http://www.utah.edu/\"
                    >University&nbsp;of&nbsp;Utah</a>&nbsp;]</li>
                </ul>
                <!-- begin copyright -->
                <span class='copyright'>
                <a href='$TBDOCBASE/docwrapper.php3?docname=copyright.html'>
                    Copyright &copy; 2000-$year The University of Utah</a>
                </span>\n";
    }
    echo "      <br>\n";
    echo "      <table class=stealth width=100% ".
	               "border='0' cellspacing='0' cellpadding='0'>";
    echo "       <tr>\n";
    
    if ($login_user) {
	echo "    <td class=reportbug>";
	echo "      <a href='$TBBASE/gotobugdb.php3".
	                    "?do=newtask&project_title=Emulab'>";
	echo "        Report Bug, Gripe, Request Feature</a>";
	echo "    </td>";
    }
    echo "        <td class=contact>Questions? Contact $TBMAILADDR</td>";
    echo "       </tr>\n";
    echo "      </table>\n";
    echo "      <!-- end copyright -->\n";
    echo "</div>";
    echo "</div>";

    ENDPAGE();

    # Plug the home site from all others.
    echo "\n<p><a href=\"www.emulab.net/netemu.php3\"></a>\n";

    # This has to be after all the tooltip definitions.
    echo "<script type='text/javascript' src='${TBBASE}/js/wz_tooltip.js'>".
	"</script>";
    echo $bodyclosestring;
    echo "\n";
    echo "</body></html>\n";
}

function PAGEERROR($msg) {
    global $drewheader, $noheaders;

    if (! $drewheader && ! $noheaders) {
	PAGEHEADER("Page Error");
    }

    echo "$msg\n";

    if (! $noheaders) 
	PAGEFOOTER();
    die("");
}

#
# Sub Page/Menu Stuff
#
function WRITESUBMENUBUTTON($text, $link, $target = "") {
    global $nextsubmenucl;

    #
    # Optional 'target' agument, so that we can pop up new windows
    #
    $targettext = "";
    if ($target) {
	$targettext = "target='$target'";
    }
    $cl = "";
    if ($nextsubmenucl != "") {
	$cl = "class='$nextsubmenucl'";
	$nextsubmenucl = "";
    }

    echo "<li $cl><a href='$link' $targettext>$text</a></li>\n";
}

function WRITESUBMENUDIVIDER() {
    global $nextsubmenucl;
    
    $nextsubmenucl = "newgroup";
}

#
# Start/End a page within a page. 
#
function SUBPAGESTART() {
    echo "<!-- begin subpage -->";
    echo "<table class=\"stealth\"
	  cellspacing='0' cellpadding='0' width='100%' border='0'>\n
            <tr>\n
              <td class=\"stealth\" valign=\"top\">\n";
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
    <h3 class="submenuheader"><?php echo "$title";?></h3>
    <ul class="submenu">
<?php
}

function SUBMENUEND() {
?>
    </ul>
    <!-- end submenu -->
  </td>
  <td class="stealth" valign='top' align='left' width='100%'>
<?php
}

# Start a new section in an existing submenu
# This includes ending the one before it
function SUBMENUSECTION($title) {
    SUBMENUSECTIONEND();
?>
      <!-- new submenu section -->
      </ul>
      <h3 class="submenuheader"><?php echo "$title";?></h3>
      <ul class="submenu">
<?php
}

# End a submenu section - only need this on the last one of the list.
function SUBMENUSECTIONEND() {
?>
      </ul>
<?php
}

# These are here so you can wedge something else under the menu in the left column.

function SUBMENUEND_2A() {
?>
    </ul>
    <!-- end submenu -->
<?php
}

function SUBMENUEND_2B() {
?>
  </td>
  <td class="stealth" valign="top" align="left" width='85%'>
<?php
}

#
# Get a view, for use with PAGEHEADER and PAGEFOOTER, for the current user
#
function GETUSERVIEW() {
    if (GETUID() && (ISPLABUSER() || isset($_REQUEST["plab_interface"]))) {
	return array('hide_sidebar' => 1, 'hide_banner' => 1,
	    'show_topbar' => "plab", 'show_bottombar' => 'plab',
	    'hide_copyright' => 1);
    } else {
	return array();
    }
}

#
# Do we view something.
#
function VIEWSET($view, $thing, $value = null) {
    if (! array_key_exists($thing, $view))
	return 0;
    if ($value) {
	return $view[$thing] == $value;
    }
    $val = $view[$thing];
    return ! empty($val);
}

function STARTBUSY($msg) {
    global $currently_busy;

    # Allow for a repeated call; Do nothing.
    if ($currently_busy)
	return;
    
    echo "<center>\n";
    echo "<b>$msg</b> ...<br>\n";
    echo "This will take a few moments; please be <em>patient</em>.<br>\n";
    echo "<img id='busy' src='busy.gif'>".
	   "<span id='loading'> Working ...</span>";
    echo "<br><br>\n";
    echo "</center>\n";
    flush();
    $currently_busy = 1;
}

function STOPBUSY() {
    global $currently_busy;

    if (!$currently_busy)
	return;

    echo "<script type='text/javascript' language='javascript'>\n";
    echo "ClearBusyIndicators('<center><b>Done!</b></center>');\n";
    echo "</script>\n";
    flush();
    $currently_busy = 0;
    sleep(1);
}

function CLEARBUSY() {
    global $currently_busy;
    
    if (!$currently_busy)
	return;

    echo "<script type='text/javascript' language='javascript'>\n";
    echo "ClearBusyIndicators('');\n";
    echo "</script>\n";
    flush();
    $currently_busy = 0;
}

function PAGEREPLACE($newpage) {
    echo "<script type='text/javascript' language='javascript'>\n";
    echo "PageReplace('$newpage');\n";
    echo "</script>\n";
    flush();
}

?>
