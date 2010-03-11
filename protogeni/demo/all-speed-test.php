<html>
<head>
</head>
<body>
<table>
<tr>

<?php

$names = explode(",", $_GET["names"]);
$cms = explode(",", $_GET["cms"]);
$hosts = explode(",", $_GET["hostnames"]);

$width = 3;

$i = 0;
for (; $i < count($names); ++$i)
{
  $url = "http://" . $hosts[$i] . "/speed-test.swf";
  if ($i != 0 && $i % $width == 0)
  {
    echo "</tr><tr>\n";
  }
?>

<td bgcolor="#ccccff" align="center">
<iframe width="380" height="350" scrolling="no" frameborder="0"
<?php
  echo "src=\"single-speed-test.php?name=" . $names[$i]
    . "&cm=" . $cms[$i] . "&hostname=" . $hosts[$i] . "\"";
?>

></iframe>

</td>

<?php
}

?>

</tr>
</table>
</body>
</html>
