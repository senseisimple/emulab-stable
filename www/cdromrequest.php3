<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#
require("defs.php3");

#
# Anyone can run this page. No login is needed.
# 

#
# Spit the form out using the array of data. 
# 
function SPITFORM($formfields, $errors)
{
    PAGEHEADER("Request a CDROM distribution");

    if ($errors) {
	echo "<table class=nogrid
                     align=center border=0 cellpadding=6 cellspacing=0>
              <tr>
                 <th align=center colspan=2>
                   <font size=+1 color=red>
                      &nbsp;Oops, please fix the following errors!&nbsp;
                   </font>
                 </td>
              </tr>\n";

	while (list ($name, $message) = each ($errors)) {
	    echo "<tr>
                     <td align=right>
                       <font color=red>$name:&nbsp;</font></td>
                     <td align=left>
                       <font color=red>$message</font></td>
                  </tr>\n";
	}
	echo "</table><br>\n";
    }

    echo "<table align=center border=1> 
          <tr>
            <td align=center colspan=2>
                Fields marked with * are required
            </td>
          </tr>\n

          <form action=cdromrequest.php3 method=post>\n";

    #
    # Full Name
    #
    echo "<tr>
              <td colspan=1>*Full Name:</td>
              <td class=left>
                  <input type=text
                         name=\"formfields[user_name]\"
                         value=\"" . $formfields[user_name] . "\"
	                 size=30>
              </td>
          </tr>\n";

    #
    # Email:
    #
    echo "<tr>
              <td colspan=1>*Email Address[<b>1</b>]:</td>
              <td class=left>
                  <input type=text
                         name=\"formfields[user_email]\"
                         value=\"" . $formfields[user_email] . "\"
	                 size=30>
              </td>
          </tr>\n";

    #
    # IP:
    #
    echo "<tr>
              <td colspan=1>*IP Address[<b>2</b>]:</td>
              <td class=left>
                  <input type=text
                         name=\"formfields[IP]\"
                         value=\"" . $formfields[IP] . "\"
	                 size=30>
              </td>
          </tr>\n";

    echo "<tr>
              <td colspan=2 align=center>
                 <b><input type=submit name=submit value=Submit></b>
              </td>
          </tr>\n";

    echo "</form>
          </table>\n";

    echo "<h4><blockquote><blockquote>
          <ol>
            <li> Please consult our
                 <a href = 'docwrapper.php3?docname=security.html'>
                 security policies</a> for information
                 regarding passwords and email addresses.
 	    <li> This address will be fixed on the CDROM and in our database.
                 You will not be able to use another IP without first
                 contacting us.
          </ol>
          </blockquote></blockquote>
          </h4>\n";
}

#
# The conclusion of a join request. See below.
# 
if (isset($finished)) {
    PAGEHEADER("Request a CDROM distribution");

    #
    # Generate some warm fuzzies.
    #
    echo "<p>
          Your request is being processed. You will receive email when
          your file is ready to download. At that point you will have 1 hour
          to retrieve the file with ftp before we remove it from the server.
          Instructions on what to do with the file will be included with the
          email message.\n";

    PAGEFOOTER();
    return;
}

#
# On first load, display a virgin form and exit.
#
if (! isset($submit)) {
    $defaults = array();
    
    SPITFORM($defaults, 0);
    PAGEFOOTER();
    return;
}

#
# Otherwise, must validate and redisplay if errors
#
$errors = array();

# Name
if (!isset($formfields[user_name]) ||
    strcmp($formfields[user_name], "") == 0) {
    $errors["Full Name"] = "Missing Field";
}

# Email
if (!isset($formfields[user_email]) ||
    strcmp($formfields[user_email], "") == 0) {
    $errors["Email Address"] = "Missing Field";
}
else {
    $user_email   = $formfields[user_email];
    $email_domain = strstr($user_email, "@");
    
    if (! $email_domain ||
	strcmp($user_email, $email_domain) == 0 ||
	strlen($email_domain) <= 1 ||
	! strstr($email_domain, ".")) {
	$errors["Email Address"] = "Looks invalid!";
    }
}

# IP
if (!isset($formfields[IP]) ||
    strcmp($formfields[IP], "") == 0) {
    $errors["IP Address"] = "Missing Field";
}
else {
    if (! ereg("^[0-9\.]+$", $formfields[IP])) {
	$errors["IP Address"] =
	    "Must be standard dotted notation";
    }
    else {
	$pieces = explode(".", $formfields[IP]);

	if (count($pieces) != 4) {
	    $errors["IP Address"] =
		"Must be standard dotted notation";
	}
    }
}

# Dump errors.
if (count($errors)) {
    SPITFORM($formfields, $errors);
    PAGEFOOTER();
    return;
}

$user_name = addslashes($formfields[user_name]);
$user_mail = $formfields[user_email];
$IP        = $formfields[IP];

#
# This page is not password protected, so anyone can run it. Its probably not
# a good idea to fire off a script for each request, especially since the
# script will likely have to lock anyway, and we do not want many dozens of
# bogus requests for CDROMS. Its also a heavyweight operation, and consumes
# a lot of disk space for each image. So, lets insert a new record into the
# DB for each request. If we get too many requests in the queue we will ask
# people to come back later? Not sure. A daemon will pick up the requests and
# process them and send back email to user. 
#
$query_result =
    DBQueryFatal("insert into cdroms ".
		 " (IP, user_name, user_email, requested) ".
		 " values ('$IP', '$user_name', '$user_email', now())");

#
# Spit out a redirect so that the history does not include a post
# in it. The back button skips over the post and to the form.
# See above for conclusion.
# 
header("Location: cdromrequest.php3?finished=1");
?>
