<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("News");

#
# If user is an admin, present edit options.
#
$uid = GETLOGIN();

if ($uid) {
    $isadmin = ISADMIN($uid);
} else {
    $isadmin = 0;
}

if ($isadmin) {
    if (isset($delete)) {
	$delete = addslashes($delete);
	echo "<center>";
	echo "<h2>Are you sure you want to delete message #$delete?</h2>";
	echo "<form action='news.php3' method='post'>\n";
	echo "<button name='deletec' value='$delete'>Yes</button>\n";
	echo "&nbsp;&nbsp;";
	echo "<button name='nothin'>No</button>\n";
	echo "</form>";
	echo "</center>";
	PAGEFOOTER();
	die("");
    }

    if (isset($deletec)) {
	$deleteConf = addslashes($deletec);

	DBQueryFatal("DELETE FROM webnews WHERE msgid='$deletec'");

	echo "<h2>Deleted message.</h2>";
	echo "<h3><a href='news.php3'>Back to news</a></h3>";
	PAGEFOOTER();
	die("");
    }

    if (isset($add)) {
	if (!isset($subject) || !strcmp($subject,"") ) {
	    # USERERROR("No subject!",1);
	    $subject = "Testbed News";
	} 
	if (!isset($author) || !strcmp($author,"") ) {
	    # USERERROR("No author!",1);
	    $author = "testbed-ops";
	} 

	if (isset($bodyfile) && 
	    strcmp($bodyfile,"") &&
	    strcmp($bodyfile,"none")) {
	    $bodyfile = addslashes($bodyfile);
	    $handle = @fopen($bodyfile,"r");
	    if ($handle) {
		$body = fread($handle, filesize($bodyfile));
		fclose($handle);
	    } else {
		USERERROR("Couldn't open uploaded file!",1);
	    }
	}

	if (!isset($body) || !strcmp($body,"")) {
	    USERERROR("No message body!",1);
	} 

	$subject = addslashes($subject);
	$author  = addslashes($author);
	$body = addslashes($body);

	if (isset($msgid)) {
	    $msgid = addslashes($msgid);
	    if (!isset($date) || !strcmp($date,"") ) {
		USERERROR("No date!",1);
	    }
	    $date = addslashes($date);
	    DBQueryFatal("UPDATE webnews SET ".
			 "subject='$subject', ".
			 "author='$author', ".
			 "date='$date', ".
			 "body='$body' ".			
			 "WHERE msgid='$msgid'");
	    echo "<h2>Updated message with subject '$subject'.</h2><br />";
	} else {	    
	    DBQueryFatal("INSERT INTO webnews (subject, date, author, body) ".
			 "VALUES ('$subject', NOW(), '$author', '$body')");
	    echo "<h2>Posted message with subject '$subject'.</h2><br />";
	}

	echo "<h3><a href='news.php3'>Back to news</a></h3>";
	PAGEFOOTER();
	die("");
    }

    if (isset($edit)) {
	$edit = addslashes($edit);
	$query_result = 
	    DBQueryFatal("SELECT subject, author, body, msgid, date ".
			 "FROM webnews ".
		         "WHERE msgid='$edit'" );

	if (!mysql_num_rows($query_result)) {
	    USERERROR("No message with msgid '$edit'!",1);
	} 

	$row = mysql_fetch_array($query_result);
	$subject = htmlspecialchars($row[subject], ENT_QUOTES);
	$date    = htmlspecialchars($row[date],    ENT_QUOTES);
	$author  = htmlspecialchars($row[author],  ENT_QUOTES);
	$body    = htmlspecialchars($row[body],    ENT_QUOTES);
	$msgid   = htmlspecialchars($row[msgid],   ENT_QUOTES);
    }

    if (isset($edit) || isset($addnew)) {	
	if (isset($addnew)) {
	    echo "<h3>Add new message:</h3>\n";
	} else {
	    echo "<h3>Edit message:</h3>\n";
	}

	echo "<form action='news.php3' ".
             "enctype='multipart/form-data' ".
             "method='post'>\n";

	if (isset($msgid)) {
	    echo "<input type='hidden' name='msgid' value='$msgid' />";
	}
	
#	if (isset($date)) {
#	    echo "<input type='hidden' name='date' value='$date' />";
#	}
	
	echo "<b>Subject:</b><br />".
	     "<input type='text' name='subject' size='50' value='$subject'>".
	     "</input><br /><br />\n";
	if (isset($date)) {
	    echo "<b>Date:</b><br />".
	         "<input type='text' name='date' size='50' value='$date'>".
		 "</input><br /><br />\n";
	}
	echo "<b>Posted by:</b><br />". 
	     "<input type='text' name='author' size='50' value='$uid'>".
	     "</input><br /><br />\n". 
	     "<b>Body (HTML):</b><br />".
       	     "<textarea cols='60' rows='25' name='body'>";

	if (isset($addnew)) {
	    echo "&lt;p&gt;\n&lt;!-- ".
		 "place message between 'p' elements ".
		 "--&gt;\n\n&lt;/p&gt;";
	} else {
	    echo $body;
	}
	echo "</textarea><br /><br />";
	echo "<b>or Upload Body File:</b><br />";
	echo "<input type='file' name='bodyfile' size='50'>";
	echo "</input>";
	echo "<br /><br />\n";
	if (isset($addnew)) {
	    echo "<button name='add'>Add</button>\n";
	} else {
	    echo "<button name='add'>Edit</button>\n";
	}
	echo "&nbsp;&nbsp;<button name='nothin'>Cancel</button>\n";
	echo "</form>";
	PAGEFOOTER();
	die("");
    } else {
	echo "<form action='news.php3' method='post'>\n";
	echo "<button name='addnew'>Add a new message</button>\n";
	echo "</form>";
    }

}

?>
<table align=center class=stealth border=0>
<tr><td class=stealth align=center><h1>News</h1></td></tr>
<tr><td class=stealth align=center>
    <a href = 'doc/docwrapper.php3?docname=changelog.html'>
    (Changelog/Technical Details)</a></td></tr>
</table>
<br />
<?php

$query_result=
    DBQueryFatal("SELECT subject, author, body, msgid, ".
		 "DATE_FORMAT(date,'%W, %M %e, %Y, %l:%i%p') as prettydate, ".
		 "(TO_DAYS(NOW()) - TO_DAYS(date)) as age ".
		 "FROM webnews ".
		 "ORDER BY date DESC" );

if (!mysql_num_rows($query_result)) {
    echo "<h4>No messages.</h4>";
} else {

    if ($isadmin) {
	echo "<form action='news.php3' method='post'>";
    }

    while ($row = mysql_fetch_array($query_result)) {
	$subject = $row[subject];
	$date    = $row[prettydate];
	$author  = $row[author];
	$body    = $row[body];
	$msgid   = $row[msgid];
	$age     = $row[age];

	echo "<table class='nogrid' 
                     cellpadding=0 
                     cellspacing=0 
                     border=0 width='100%'>";

	echo "<tr><th style='padding: 4px;'><font size=+1>$subject</font>";

	if ($age < 7) {
	    echo "&nbsp;<img border=0 src='new.gif' />";
	}

	echo "</th></tr>\n".
	     "<tr><td style='padding: 4px; padding-top: 2px;'><font size=-1>";
	echo "<b>$date</b>; posted by <b>$author</b>.";

	if ($isadmin) {
	    echo " (Message <b>#$msgid</b>)";
	}

	echo "</font></td></tr><tr><td style='padding: 4px; padding-top: 2px;'>".
	     "<div style='background-color: #FFFFF2; ".
	     "border: 1px solid #AAAAAA; ".
      	     "padding: 6px'>".
	     $body.
	     "</div>".
	     "</td></tr>";

	if ($isadmin) {
	    echo "<tr><td>";
	    echo "<button name='edit' value='$msgid'>".
		 "Edit</button>".
	         "<button name='delete' value='$msgid'>".
		 "Delete</button>\n";
	    echo "</td></tr>";
	}
	echo "</table><br />";
    } 

    if ($isadmin) { 
	echo "</form>\n"; 
    }
}		  

PAGEFOOTER();
?>

