<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#

#
# This is an included file. No headers or footers or includes!
# 
$query_result = DBQueryFatal("SELECT * FROM projects WHERE pid='$pid'");
if (mysql_num_rows($query_result) == 0) {
  USERERROR("The project $pid is not a valid project.", 1);
}
$row = mysql_fetch_array($query_result);

echo "<center>
      <h3>Project Information</h3>
      </center>
      <table align=center border=1>\n";

$proj_created	= $row[created];
$proj_expires	= $row[expires];
$proj_name	= $row[name];
$proj_URL	= $row[URL];
$proj_funders	= $row[funders];
$proj_head_uid	= $row[head_uid];
$proj_members   = $row[num_members];
$proj_pcs       = $row[num_pcs];
$proj_sharks    = $row[num_sharks];
$proj_why       = $row[why];
$control_node	= $row[control_node];

#
# Generate the table.
# 
echo "<tr>
          <td>Name: </td>
          <td class=\"left\">$pid</td>
      </tr>\n";

echo "<tr>
          <td>Long Name: </td>
          <td class=\"left\">$proj_name</td>
      </tr>\n";

echo "<tr>
          <td>Project Head: </td>
          <td class=\"left\">$proj_head_uid</td>
      </tr>\n";

echo "<tr>
          <td>URL: </td>
          <td class=\"left\">
              <A href='$proj_URL'>$proj_URL</A></td>
      </tr>\n";

echo "<tr>
          <td>Funders: </td>
          <td class=\"left\">$proj_funders</td>
      </tr>\n";

echo "<tr>
          <td>#Project Members: </td>
          <td class=\"left\">$proj_members</td>
      </tr>\n";

echo "<tr>
          <td>#PCs: </td>
          <td class=\"left\">$proj_pcs</td>
      </tr>\n";

echo "<tr>
          <td>#Sharks: </td>
          <td class=\"left\">$proj_sharks</td>
      </tr>\n";

echo "<tr>
          <td>Created: </td>
          <td class=\"left\">$proj_created</td>
      </tr>\n";

echo "<tr>
          <td>Expires: </td>
          <td class=\"left\">$proj_expires</td>
      </tr>\n";

echo "<tr>
          <td colspan='2'>Why?:</td>
      </tr>\n";

echo "<tr>
          <td colspan='2' width=600>$proj_why</td>
      </tr>\n";

echo "</table>\n";


$userinfo_result =
    DBQueryFatal("SELECT * from users where uid='$proj_head_uid'");

$row	= mysql_fetch_array($userinfo_result);
$usr_expires = $row[usr_expires];
$usr_email   = $row[usr_email];
$usr_URL     = $row[usr_URL];
$usr_addr    = $row[usr_addr];
$usr_name    = $row[usr_name];
$usr_phone   = $row[usr_phone];
$usr_title   = $row[usr_title];
$usr_affil   = $row[usr_affil];

echo "<center>
      <h3>Project Leader Information</h3>
      </center>
      <table align=center border=1>\n";

echo "<tr>
          <td>Username:</td>
          <td>$proj_head_uid</td>
      </tr>\n";

echo "<tr>
          <td>Full Name:</td>
          <td>$usr_name</td>
      </tr>\n";

echo "<tr>
          <td>Email Address:</td>
          <td>$usr_email</td>
      </tr>\n";

echo "<tr>
          <td>Home Page URL:</td>
          <td><A href='$usr_URL'>$usr_URL</A></td>
      </tr>\n";

echo "<tr>
          <td>Expiration date:</td>
          <td>$usr_expires</td>
      </tr>\n";

echo "<tr>
          <td>Mailing Address:</td>
          <td>$usr_addr</td>
      </tr>\n";

echo "<tr>
          <td>Phone #:</td>
          <td>$usr_phone</td>
      </tr>\n";

echo "<tr>
          <td>Title/Position:</td>
          <td>$usr_title</td>
     </tr>\n";

echo "<tr>
          <td>Institutional Affiliation:</td>
          <td>$usr_affil</td>
      </tr>\n";

echo "</table>\n";

#
# This is an included file.
# 
?>
