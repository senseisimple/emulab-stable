<html>
<head>
<title>Foo</title>
</head>
<body bgcolor="#ffffff">
<H1>Utah Testbed Machine Status</h1>
<P>
<? 
  mysql_connect("localhost", "webuser", ""); 
  $query = "SELECT n.node_id, n.type, j.eid from nodes as n left join reserved AS j ON n.node_id = j.node_id";
  $result = mysql_db_query("tbdb", $query);
  if (!$result) {
	$err = mysql_error();
	echo "<H1>Could not query the database: $err</h1>\n";
        exit;
  }
  echo "<table border=1 padding=1>\n";
  echo "<tr><td><b>ID</b></td> <td><b>Type</b></td> <td><b>Reservation Status</b></td></tr>\n";
  while ($r = mysql_fetch_array($result)) {
	$id = $r["node_id"];  $type = $r["type"]; 
	$res = $r["eid"];
	if (!$res || $res == "NULL") {
		$res = " ";
	}
	echo "<tr><td>$id</td> <td>$type</td> <td>$res</td></tr>\n";
  }
  echo "</table>\n";
?>
</body>
</html>
