<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2004 University of Utah and the Flux Group.
# All rights reserved.
#
chdir("..");
include("defs.php3");

#
# Prices hardwired for now
#
$mobo_price   = 0.69;
$dboard_price = 5369896.0;

#
# No PAGEHEADER since we spit out a Location header later. See below.
#

# This comes in as a GET argument all the time.
unset($order_id);

if (isset($_GET['order_id'])) {
    $order_id = $_GET['order_id'];
}

#
# Spit the form out using the array of data. 
# 
function SPITFORM($formfields, $errors, $order_id, $justview)
{
    global $mobo_price, $dboard_price;
    
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

    echo "<SCRIPT LANGUAGE=JavaScript>
          <!--
              function NormalSubmit(theform) {
                  theform.target='_self';
                  theform.submit();
              }
              function ChangeQuantities(theform) {
                  theform.target='_self';
		  theform['quantitychange'].value = 'yep';
                  theform.submit();
              }
          //-->
          </SCRIPT>\n";

    echo "<table align=center border=1>\n";
    if (! $justview) {
	echo "<form enctype=multipart/form-data
                    onsubmit=\"return false;\"
                    name=myform
                    action=preorder.php" .
	                ($order_id ? "?order_id=$order_id" : "") .
	                " method=post>\n";

        # Something funky going on ...
	echo "<input type=hidden name=fakesubmit value=Submit>\n";
	echo "<input type=hidden name=quantitychange value=no>\n";
    }

    #
    # Existing order_id.
    #
    if ($order_id) {
	echo "<tr>
                  <td colspan=1>Order ID:</td>
                  <td class=left>$order_id</td>
              </tr>\n";
    }

    #
    # Full Name
    #
    echo "<tr>
              <td colspan=1>Full Name:</td>
              <td class=left>";
    if ($justview)
	echo      $formfields[usr_name];
    else
	echo      "<input type=text
                         name=\"formfields[usr_name]\"
                         value=\"" . $formfields[usr_name] . "\"
	                 size=30>";
    echo "    </td>
          </tr>\n";

    #
    # Email:
    #
    echo "<tr>
              <td colspan=1>Email Address:</td>
              <td class=left>";
    if ($justview)
	echo      $formfields[usr_email];
    else
	echo "    <input type=text
                         name=\"formfields[usr_email]\"
                         value=\"" . $formfields[usr_email] . "\"
                         size=30>";
    echo "    </td>
          </tr>\n";

    #
    # Phone
    #
    echo "<tr>
              <td colspan=1>Phone #:</td>
              <td class=left>";
    if ($justview)
	echo      $formfields[usr_phone];
    else
	echo "    <input type=text
                         name=\"formfields[usr_phone]\"
                         value=\"" . $formfields[usr_phone] . "\"
	                 size=15>";
    echo "    </td>
          </tr>\n";


    #
    # Affiliation:
    # 
    echo "<tr>
              <td colspan=1>Institutional<br>Affiliation (if any):</td>
              <td class=left>";
    if ($justview)
	echo      $formfields[usr_affil];
    else
	echo "    <input type=text
                         name=\"formfields[usr_affil]\"
                         value=\"" . $formfields[usr_affil] . "\"
	                 size=40>";
    echo "    </td>
          </tr>\n";

    #
    # Parts
    # 
    echo "<tr>
              <td colspan=2 align=center>USRP Parts Quantities</td>
          </tr>\n";

    echo "<tr>
              <td colspan=1>USRP Motherboard:</td>
              <td class=left>";
    if ($justview)
	echo      $formfields[num_mobos];
    else
	echo "    <input type=text
                         name=\"formfields[num_mobos]\"
                         onChange='ChangeQuantities(myform);'
                         value=\"" . $formfields[num_mobos] . "\"
	                 size=3>";
    echo "        (\$${mobo_price} each)";
    echo "    </td>
          </tr>\n";

    echo "<tr>
              <td colspan=1>420MHZ Daughterboards:</td>
              <td class=left>";
    if ($justview)
	echo      $formfields[num_dboards];
    else
	echo "    <input type=text
                         name=\"formfields[num_dboards]\"
                         onChange='ChangeQuantities(myform);'
                         value=\"" . $formfields[num_dboards] . "\"
	                 size=3>";
    echo "        (\$${dboard_price} each)";
    echo "    </td>
          </tr>\n";

    #
    # A totals field. Silly.
    #
    $total_cost = 0.0;
    if ($formfields[num_mobos])
	$total_cost += ($formfields[num_mobos] * $mobo_price);
    if ($formfields[num_dboards])
	$total_cost += ($formfields[num_dboards] * $dboard_price);
    
    echo "<tr>
              <td height=2 colspan=2></td>
          </tr>\n";

    echo "<tr>
              <td colspan=1 align=right><b>Total Cost</b>:</td>
              <td class=left><b>\$${total_cost}</b></td>
          </tr>\n";

    echo "<tr>
              <td height=2 colspan=2></td>
          </tr>\n";

    #
    # Intended Use
    #
    if ($justview) {
	echo "<tr>
                  <td colspan=2 align=center>
                      Your intended use
                  </td>
              </tr>
              <tr>
                  <td colspan=2 class=left>" .
	                   $formfields[intended_use] . 
                  "</td>
              </tr>\n";
    }
    else {
	echo "<tr>
                  <td colspan=2 align=center>
                      Please describe your intended use
                  </td>
              </tr>
              <tr>
                  <td colspan=2 align=center class=left>
                      <textarea name=\"formfields[intended_use]\"
                                rows=3 cols=60>" .
	                   ereg_replace("\r", "", $formfields[intended_use]) .
	             "</textarea>
                  </td>
              </tr>\n";
    }

    #
    # General Comments
    #
    if ($justview) {
	if (isset($formfields[comments]) && $formfields[comments] != "") {
	    echo "<tr>
                      <td colspan=2 align=center>
                          Optional Comments
                      </td>
                  </tr>
                  <tr>
                      <td colspan=2 class=left>" .
	                       $formfields[comments] . 
                      "</td>
                  </tr>\n";
	}
    }
    else {
	echo "<tr>
                  <td colspan=2 align=center>
                      Any other comments you would like to make?
                  </td>
              </tr>
              <tr>
                  <td colspan=2 align=center class=left>
                      <textarea name=\"formfields[comments]\"
                                rows=5 cols=60>" .
	                   ereg_replace("\r", "", $formfields[comments]) .
	             "</textarea>
                  </td>
              </tr>\n";
    }

    if (! $justview) {
	echo "<tr>
                  <td align=center colspan=2>
                     <b><input type=button value=Submit name=fakesubmit
                               onclick=\"NormalSubmit(myform);\"></b>
                  </td>
              </tr>\n";
	echo "</form>";
    }
    echo "</table>\n";
}

# For initial order and for order modify.
$defaults = array();

#
# Check for existing order ID, and pull that from the DB for the defaults
# array.
# 
if (isset($order_id) && $order_id != "") {
    if (! preg_match("/^[\w]+$/", $order_id)) {
	PAGEARGERROR();
    }

    $query_result =
	DBQueryFatal("select * from usrp_orders where order_id='$order_id'");

    if (! mysql_num_rows($query_result)) {
	USERERROR("No such USRP order id '$order_id'", 1);
    }
    $row = mysql_fetch_array($query_result);

    $defaults["usr_name"]     = $row["name"];
    $defaults["usr_email"]    = $row["email"];
    $defaults["usr_phone"]    = $row["phone"];
    $defaults["usr_affil"]    = $row["affiliation"];
    $defaults["intended_use"] = $row["intended_use"];
    $defaults["comments"]     = $row["comments"];
    $defaults["num_mobos"]    = $row["num_mobos"];
    $defaults["num_dboards"]  = $row["num_dboards"];
}
else {
    $defaults["num_mobos"]    = 0;
    $defaults["num_dboards"]  = 0;
}

#
# The conclusion of a pre-order, or just wanting to view an order.
# 
if (isset($_GET['finished']) || isset($_GET['viewonly'])) {
    PAGEHEADER("Pre-Order USRP Parts");

    if (isset($_GET['finished'])) {
        #
        # Generate some warm fuzzies.
        #
	echo "<center><font size=+1>
               Thank you for placing your pre-order.<br>We will notify you via
               email when you can place your actual order.
              </font></center><br>\n";
    }
    
    # Spit out order in viewmode only.
    SPITFORM($defaults, 0, $order_id, 1);

    echo "<br><center>
          Would you like to <a href=preorder.php?order_id=$order_id>
            <b>edit</b></a> this order?</center>\n";
    PAGEFOOTER();
    return;
}

#
# On first load, display a virgin form and exit.
#
if (! isset($_POST['fakesubmit'])) {
    PAGEHEADER("Pre-Order USRP Parts");
    SPITFORM($defaults, 0, $order_id, 0);
    PAGEFOOTER();
    return;
}
else {
    # Form submitted. Make sure we have a formfields array and a target_uid.
    if (!isset($_POST['formfields']) ||
	!is_array($_POST['formfields'])) {
	PAGEARGERROR("Invalid form arguments.");
    }
    $formfields = $_POST['formfields'];

    #
    # If this was quantities change, just redisplay with current info
    # so that the price is updated.
    #
    if (isset($_POST['quantitychange']) && $_POST['quantitychange'] == "yep") {
	PAGEHEADER("Pre-Order USRP Parts");
	SPITFORM($formfields, 0, $order_id, 0);
	PAGEFOOTER();
	return;
    }
}

#
# Otherwise, must validate and redisplay if errors
#
$errors = array();

#
# User Name
#
if (!isset($formfields[usr_name]) ||
    strcmp($formfields[usr_name], "") == 0) {
    $errors["Full Name"] = "Missing Field";
}
elseif (! TBvalid_usrname($formfields[usr_name])) {
    $errors["Full Name"] = TBFieldErrorString();
}

#
# User email
# 
if (!isset($formfields[usr_email]) ||
    strcmp($formfields[usr_email], "") == 0) {
    $errors["Email Address"] = "Missing Field";
}
elseif (! TBvalid_email($formfields[usr_email])) {
    $errors["Email Address"] = TBFieldErrorString();
}

#
# User Phone
# 
if (!isset($formfields[usr_phone]) ||
    strcmp($formfields[usr_phone], "") == 0) {
    $errors["Phone #"] = "Missing Field";
}
elseif (!TBvalid_phone($formfields[usr_phone])) {
    $errors["Phone #"] = TBFieldErrorString();
}

#
# Affiliation
# 
if (!isset($formfields[usr_affil])) {
    $formfields[usr_affil] = "";
}
elseif (! TBvalid_affiliation($formfields[usr_affil])) {
    $errors["Affiliation"] = TBFieldErrorString();
}

#
# Parts.
#
if (!isset($formfields[num_mobos]) ||
    strcmp($formfields[num_mobos], "") == 0) {
    $errors["#of Motherboards"] = "Missing Field";
}
elseif (! TBvalid_num_pcs($formfields[num_mobos])) {
    $errors["#of Motherboards"] = TBFieldErrorString();
}

if (!isset($formfields[num_dboards]) ||
    strcmp($formfields[num_dboards], "") == 0) {
    $errors["#of Daughterboards"] = "Missing Field";
}
elseif (! TBvalid_num_pcs($formfields[num_dboards])) {
    $errors["#of Daughterboards"] = TBFieldErrorString();
}

#
# Intended Use
#
if (!isset($formfields[intended_use]) ||
    strcmp($formfields[intended_use], "") == 0) {
    $errors["Intended Use"] = "Missing Field";
}
elseif (! TBvalid_why($formfields[intended_use])) {
    $errors["Intended Use"] = TBFieldErrorString();
}

#
# Intended Use
#
if (!isset($formfields[comments])) {
    $formfields[comments] = "";
}
elseif (! TBvalid_why($formfields[comments])) {
    $errors["Comments"] = TBFieldErrorString();
}

if (count($errors)) {
    PAGEHEADER("Pre-Order USRP Parts");
    SPITFORM($formfields, $errors, $order_id, 0);
    PAGEFOOTER();
    return;
}

#
# Certain of these values must be escaped or otherwise sanitized.
#
$usr_name          = addslashes($formfields[usr_name]);
$usr_email         = $formfields[usr_email];
$usr_phone         = $formfields[usr_phone];
$usr_affil         = addslashes($formfields[usr_affil]);
$intended_use      = addslashes($formfields[intended_use]);
$comments          = addslashes($formfields[comments]);
$num_mobos         = $formfields[num_mobos];
$num_dboards       = $formfields[num_dboards];

#
# Insert order into the DB, or update current entry.
#
if (isset($order_id)) {
    DBQueryFatal("update usrp_orders set ".
		 "  name='$usr_name', ".
		 "  email='$usr_email', ".
		 "  phone='$usr_phone', ".
		 "  affiliation='$usr_affil', ".
		 "  intended_use='$intended_use', ".
		 "  comments='$comments', ".
		 "  num_mobos='$num_mobos', ".
		 "  num_dboards='$num_dboards', ".
		 "  modify_date=now() ".
		 "where order_id='$order_id'");
    $action = "modified";
}
else {
    $order_id = md5(uniqid(rand(),1));

    DBQueryFatal("insert into usrp_orders ".
		 " (order_id, name, email, phone, affiliation, intended_use, ".
		 "  comments, num_mobos, num_dboards, ".
		 "  order_date, modify_date) ".
		 "values ".
		 " ('$order_id', '$usr_name', '$usr_email', '$usr_phone', ".
		 "  '$usr_affil', '$intended_use', '$comments', ".
		 "  $num_mobos, $num_dboards, ".
		 "  now(), now())");
    
    $action = "placed";
}

$total_cost = 0.0;
if ($num_mobos) {
    $total_cost += ($num_mobos * $mobo_price);
}
if ($num_dboards) {
    $total_cost += ($num_dboards * $dboard_price);
}

#
# Send email to someone.
# 
TBMAIL("$usr_name <$usr_email>",
       "USRP Preorder $order_id",
       "A USRP preorder has been $action by $usr_name ($usr_email).\n".
       "\n".
       "Order Info:\n".
       "Order ID:        $order_id\n".
       "Name:            $usr_name\n".
       "Email:           $usr_email\n".
       "Phone:           $usr_phone\n".
       "Affiliation:     $usr_affil\n".
       "#Motherboards:   $num_mobos\n".
       "#Daughterboards: $num_dboards\n".
       "Total Cost:      $total_cost\n".
       "Intended Use:\n".
       "$intended_use\n".
       "Comments:\n".
       "$comments\n".
       "\n".
       "Thank you for your preorder. You may modify your order by going to:\n".
       "\n".
       "    ${TBBASE}/usrp/preorder.php?order_id=$order_id\n".
       "\n".
       "Thank you very much!\n",
       "From: $TBMAIL_OPS\n".
       "Bcc: $TBMAIL_OPS\n".
       "Errors-To: $TBMAIL_WWW");

#
# Spit out a redirect so that the history does not include a post
# in it. The back button skips over the post and to the form.
# See above for conclusion.
# 
header("Location: preorder.php?finished=1&order_id=$order_id");

?>
