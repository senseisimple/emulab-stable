<?php header("Pragma: no-cache");?>

<html>
  <head>

<?php
  if (getenv("HTTPS") != "on") {
    $title_header = "Redirecting to HTTPS";
    $body_header  = $title_header;
    $to_where     = "https://$SERVER_NAME$PHP_SELF";
    echo "<META HTTP-EQUIV=Refresh CONTENT='0; URL=$to_where'>";
  } else {
    include "webdb_backend.php3";
  }
?>    

    <title>WebDb - <?php echo $title_header ?></title>

    <style type="text/css"><!--
      p.location {
	color: #11bb33;
	font-size: small;
      }
      body {
	background-color: #EEFFEE;
      }
      h1 {
	color: #004400;
      }
      th {
	background-color: #AABBAA;
	color: #000000;
	font-size: x-small;
      }
      td {
	background-color: #D4E5D4;
	font-size: x-small;
      }
      form {
	margin-top: 0;
	margin-bottom: 0;
      }
      a {
	text-decoration:none;
	color: #248200;
      }
      a:link {}
      a:hover {
	ba2ckground-color:#EEEFD5;
	color:#54A232;
	text-decoration:underline;               
      }
      //-->
 </style>


  </head>
  <body>
    <h1><?php echo $body_header ?></h1> 
    <?php echo $body?>
    <hr><p>based on mysql.php3 by SooMin Kim.</p>
  </body>
</html>


















