<html>
<head>
<title>Utah Testbed Project Request</title>
<link rel='stylesheet' href='tbstyle.css' type='text/css'>
</head>
<body>
<?php
#
# First off, sanity check the form to make sure all the required fields
# were provided. I do this on a per field basis so that we can be
# informative. Be sure to correlate these checks with any changes made to
# the project form. Note that this sequence of  statements results in
# only the last bad field being displayed, but thats okay. The user will
# eventually figure out that fields marked with * mean something!
#
$formerror="No Error";
if (!isset($gid) ||
    strcmp($gid, "TestNet-One") == 0) {
  $formerror = "Name";
}
if (!isset($grp_head_uid) ||
    strcmp($grp_head_uid, "") == 0) {
  $formerror = "Username";
}
if (!isset($grp_name) ||
    strcmp($grp_name, "Test Networks One") == 0) {
  $formerror = "Long Name";
}
if (!isset($usr_name) ||
    strcmp($usr_name, "") == 0) {
  $formerror = "Full Name";
}
if (!isset($grp_URL) ||
    strcmp($grp_URL, "http://www.testnetworks.org") == 0) {
  $formerror = "URL";
}
if (!isset($email) ||
    strcmp($email, "") == 0) {
  $formerror = "Email Address";
}
if (!isset($usr_addr) ||
    strcmp($usr_addr, "") == 0) {
  $formerror = "Mailing Address";
}
if (!isset($grp_affil) ||
    strcmp($grp_affil, "UofX Networks Group") == 0) {
  $formerror = "Research Afilliation";
}
if (!isset($usr_phones) ||
    strcmp($usr_phones, "") == 0) {
  $formerror = "Phone #";
}
#
# Not sure about the passwd. If the user is already known, then is he
# supposed to plug his passwd in?
#
if ((!isset($password1) || strcmp($password1, "") == 0) ||
    (!isset($password2) || strcmp($password2, "") == 0)) {
  $formerror = "Password";
}

if ($formerror != "No Error") {
  echo "<h3><br><br>
        Missing field; Please go back and fill out the \"$formerror\" field!\n
        </h3>
        </body>
        </html>";
  die("");
}

$returning=0;
$my_passwd = $password1;
$mypipe = popen(escapeshellcmd(
    "/usr/testbed/bin/checkpass $my_passwd $grp_head_uid '$usr_name:$email'"),
    "w+");
if ($mypipe) {
  $retval=fgets($mypipe,1024);
  if (strcmp($retval,"ok\n")!=0) {
    die("<h3>The password you have chosen will not work:<p>$retval</h3>");
  }
} else {
  mail("testbed-www@flux.cs.utah.edu","TESTBED: checkpass failure",
       "\n$usr_name ($grp_head_uid) just tried to set up a testbed account,\n".
       "but checkpass pipe did not open (returned '$mypipe').\n".
       "\nThanks\n");
}
$enc = crypt("$my_passwd");
array_walk($HTTP_POST_VARS, 'addslashes');
if (isset($gid) && isset($password1) && isset($email) && 
    (($password1 == $password2) || ($enc == $password2))) {
# echo "GOT PWD crypt = $enc";
  $query = "SELECT usr_pswd FROM users WHERE uid=\"$grp_head_uid\"";
  $result = mysql_db_query("tbdb", $query);
  $query2 = "SELECT gid FROM groups WHERE gid=\"$gid\"";
  $result2 = mysql_db_query("tbdb", $query2);
  if ($row = mysql_fetch_row($result2)) {
    die("<h3>The project name you have chosen is already in use. ".
	"Please select another. If you are a returning user, you must ".
	"log in and use your current password.</h3>");
  } elseif ($row = mysql_fetch_row($result)) {
    #returning user, making new group
    $usr_pswd = $row[0];
    if ($usr_pswd != $enc) {
      die("<H3>The username that you have chosen is already in use. ".
	  "Please select another. If you are a returning user, you must ".
	  "log in and use your current password.</h3>\n");
    }
    $returning=1;
  } else { #The uid and gid are not already in use, and user is new
    $query3 = "SELECT unix_uid FROM users ORDER BY unix_uid DESC";
    $result3 = mysql_db_query("tbdb", $query2);
    $row = mysql_fetch_row($result3);
    $unix_uid = $row[0];
    ++$unix_uid;
    $cmnd1 = "INSERT INTO users ".
       "(uid,usr_created,usr_expires,usr_name,usr_email,usr_addr,".
       "usr_phone,usr_pswd,unix_uid,status) ".
       "VALUES ('$grp_head_uid',now(),'$grp_expires','$usr_name',".
       "'$email','$usr_addr','$usr_phones','$enc','$unix_uid','newuser')";
    $cmndres1 = mysql_db_query("tbdb", $cmnd1);
    if (!$cmndres1) {
      $err = mysql_error();
      echo "<H3>Failed to add user $grp_head_uid to the database: $err</h3>\n";
      exit;
    }
  }
  $ques = "SELECT unix_gid FROM groups ORDER BY unix_gid DESC";
  $resp = mysql_db_query("tbdb", $ques);
  $row = mysql_fetch_row($resp);
  $unix_gid = $row[0];
  ++$unix_gid;
  $cmnd2 = "INSERT INTO groups (gid,grp_created,grp_expires,grp_name,".
     "grp_URL,grp_affil,grp_addr,grp_head_uid,cntrl_node,unix_gid)".
     "VALUES ('$gid',now(), '$grp_expires','$grp_name','$grp_URL',".
     "'$grp_affil','$grp_addr','$grp_head_uid', '','$unix_gid')";
  $cresult = mysql_db_query("tbdb", $cmnd2);
  if (!cresult) {
    $err = mysql_error();
    echo "<H3>Failed to add project $gid to the database: $err</h3>\n";
    exit;
  }
  mysql_db_query("tbdb","insert into grp_memb (uid,gid,trust)".
		 "values ('$grp_head_uid','$gid','none');");
  $fp = fopen("/usr/testbed/www/maillist/leaders.txt", "a");
  $fp2 = fopen("/usr/testbed/www/maillist/users.txt", "a");
  fwrite($fp, "$email\n");   #Writes the email address to mailing lists
  fwrite($fp2, "$email\n");
#  mail("lepreau@cs.utah.edu,calfeld@cs.utah.edu", 
  mail("newbold@cs.utah.edu,stoller@cs.utah.edu,lepreau@cs.utah.edu", 
       "TESTBED: New Group", "'$usr_name' wants to start group ".
       "'$gid'.\nContact Info:\nName:\t\t$usr_name ($grp_head_uid)\n".
       "Email:\t\t$email\nGroup:\t\t$grp_name\nURL:\t\t$grp_URL\n".
       "Affiliation:\t$grp_affil\nAddress:\t$grp_addr\n".
       "Phone:\t\t$usr_phones\n\n".
       "Reasons:\n$why\n\nPlease review the application and when you have\n".
       "made a decision, go to <https://plastic.cs.utah.edu/tbdb.html> and\n".
       "select the 'Group Approval' page.\n\nThey are expecting a result ".
       "within 72 hours.\n", 
       "From: $usr_name <$email>\nCc: testbed-www@flux.cs.utah.edu\n".
       "Errors-To: testbed-www@flux.cs.utah.edu");
  if (! $returning) {
    mail("$email","TESTBED: Your New User Key",
	 "\nDear $usr_name:\n\n\tThank you for applying to use the Utah ".
	 "Network Testbed. As promised,\nhere is your key to verify your ".
	 "account. Your key is:\n\n".
	 crypt("TB_".$grp_head_uid."_USR",strlen($grp_head_uid)+13)."\n\n\t Please ".
	 "return to <https://plastic.cs.utah.edu/tbdb.html> and log in,\nusing ".
	 "the user name and password you gave us when you applied. You will\n".
	 "then find an option on the menu called 'New User Verification'. ".
	 "Select it,\nand on that page enter in your user name, password, and ".
	 "your key,\nand you will be verified as a user. When you have been ".
	 "both verified and\napproved by the Approval Committee, you will be ".
	 "marked as an active user,\nand will be granted full access to your ".
  	 "user account.\n\nThanks,\nTestbed Control\nUtah Network Testbed\n",
         "From: Testbed Control <testbed-control@flux.cs.utah.edu>\n".
         "Cc: Testbed WWW <testbed-www@flux.cs.utah.edu>\n".
         "Errors-To: Testbed WWW <testbed-www@flux.cs.utah.edu>");
  }
  echo "
<H1>Project '$gid' successfully added.</h1>
<h2>The review committee has been notified of your application.
Most applications are reviewed within one week. We will notify
you by e-mail at '$usr_name&nbsp;&lt;$email>' of their decision
regarding your proposed project '$gid'.\n";
  if (! $returning) {
    echo "
<p>In the meantime, for
security purposes, you will receive by e-mail a key. When you
receive it, come back to the site, and log in. When you do, you
will see a new menu option called 'New User Verification'. On
that page, enter in your username, password, and the key,
exactly as you received it in your e-mail. You will then be
marked as a verified user.
<p>Once you have been both verified
and approved, you will be classified as an active user, and will 
be granted full access to your user account.</h2>
";
  }
} else { #if not enough information was given
  echo "<H3>Please return to <A href=\"addgrp.php3\">the form</A>
and enter your user ID and/or email address and/or password.</h3>\n";
} 
?>
    </body>
    </html>
    


    
