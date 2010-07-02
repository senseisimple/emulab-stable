<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2004, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
chdir("..");
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("USRP Preorder List");

#
# Only known and logged in users allowed.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

if (! $isadmin) {
    USERERROR("You do not have permission to view the USRP preorder list!", 1);
}

#
# Get the list. Date order, most recent first.
#
$query_result =
    DBQueryFatal("select * from usrp_orders ".
		 "order by order_date desc");


if (($count = mysql_num_rows($query_result)) == 0) {
    USERERROR("There are no USRP preorders!", 1);
}

#
# Grab some totals for the top of the page.
#
$total_mobos   = 0;
$total_dboards = 0;

while ($row = mysql_fetch_array($query_result)) {
    $num_mobos    = $row["num_mobos"];
    $num_dboards  = $row["num_dboards"];

    $total_mobos   += $num_mobos;
    $total_dboards += $num_dboards;
}
mysql_data_seek($query_result, 0);

echo "<center>
       <font size=+1>There are $count USRP preorders</font><br><br>\n";

echo "<table align=center border=2 cellpadding=5 cellspacing=2>
      <tr>
         <td>Total Motherboards:</td>
         <td align=left>$total_mobos</td>
      </tr>
      <tr>
         <td>Total Daughterboards:</td>
         <td align=left>$total_dboards</td>
      </tr>
      </table>\n";
echo "</center><br>\n";

echo "<table width=\"100%\" border=2 cellpadding=1 cellspacing=2
       align='center'>\n";


echo "<tr>
          <th rowspan=2>Order ID</th>
          <th rowspan=2>Order Date</th>
          <th rowspan=2>Name</th>
          <th>E-mail</th>
          <th>#Mobos</th>
      </tr>
      <tr>
          <th>Phone</th>
          <th>#Dboards</th>
      </tr>\n";

while ($row = mysql_fetch_array($query_result)) {
    $order_id     = $row["order_id"];
    $usr_name     = $row["name"];
    $usr_email    = $row["email"];
    $usr_phone    = $row["phone"];
    $usr_affil    = $row["affiliation"];
    $intended_use = $row["intended_use"];
    $comments     = $row["comments"];
    $num_mobos    = $row["num_mobos"];
    $num_dboards  = $row["num_dboards"];
    $order_date   = $row["order_date"];


    echo "<tr>
              <td height=5 colspan=5></td>
          </tr>\n";

    echo "<tr>
              <td rowspan=2>
                  <a href=temp-preorder.php?order_id=$order_id&viewonly=1>
                     $order_id</a></td>
              <td rowspan=2>$order_date</td>
              <td rowspan=2>$usr_name</td>
              <td>$usr_email</td>
              <td>$num_mobos</td>
          </tr>
          <tr>
              <td>$usr_phone</td>
              <td>$num_dboards</td>
          </tr>\n";
}
echo "</table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
