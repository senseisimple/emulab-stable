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
    PAGEHEADER("Request a CDROM key");

    echo "<blockquote>
          Please enter your name and email address. We will send you a
          key via email that you can use when installing the CD
          on your machine. This key is good for one installation;
          if you want to install the CD on multiple machines, you will
          need a separate key for each install. New keys will lapse
          after 48 hours if not used. 
          </blockquote><br>\n";

    if ($errors) {
	echo "<table align=center border=0 cellpadding=0 cellspacing=2>
              <tr>
                 <td nowrap align=center colspan=3>
                   <font size=+1 color=red>
                      Oops, please fix the following errors!
                   </font>
                 </td>
              </tr>\n";

	while (list ($name, $message) = each ($errors)) {
	    echo "<tr>
                     <td align=right><font color=red>$name:</font></td>
                     <td>&nbsp &nbsp</td>
                     <td align=left><font color=red>$message</font></td>
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

          <form action=cdromnewkey.php method=post>\n";

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
          </ol>
          </blockquote></blockquote>
          </h4>\n";
}

#
# The conclusion of a join request. See below.
# 
if (isset($finished)) {
    PAGEHEADER("Request a CDROM Key");

    #
    # Generate some warm fuzzies.
    #
    echo "<p>
          Your request is being processed. You will receive email 
          with your new key in a few moments. Your key is good for one
          installation, and will lapse after 48 hours if not used.
          If you do not receive email notification within a reasonable amount
          of time, please contact $TBMAILADDR.\n";

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

# Dump errors.
if (count($errors)) {
    SPITFORM($formfields, $errors);
    PAGEFOOTER();
    return;
}

$user_name = addslashes($formfields[user_name]);
$user_mail = $formfields[user_email];
$newkey    = substr(GENHASH(), 0, 16);
$chunked   = chunk_split($newkey, 4, " ");

$query_result =
    DBQueryFatal("insert into widearea_privkeys ".
		 " (privkey, user_name, user_email, requested) ".
		 " values ('$newkey', '$user_name', '$user_email', now())");

TBMAIL("$user_name <$user_email>",
       "Your CD key",
       "\n".
       "Dear $user_name:\n\n".
       "This is your key to unlock your CD:\n\n".
       "          $chunked\n\n".
       "Please enter this key when installing your CD.\n".
       "This key is good for one installation, and will lapse after 48 hours.".
       "\n\n".
       "Thanks,\n".
       "Testbed Ops\n".
       "Utah Network Testbed\n",
       "From: $TBMAIL_OPS\n".
       "Bcc: $TBMAIL_AUDIT\n".
       "Errors-To: $TBMAIL_WWW");

#
# Spit out a redirect so that the history does not include a post
# in it. The back button skips over the post and to the form.
# See above for conclusion.
# 
header("Location: cdromnewkey.php?finished=1");
?>
