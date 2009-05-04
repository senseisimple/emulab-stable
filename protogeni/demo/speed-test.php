<html>
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
<h3>

<?php
  echo $cms[$i];
?>

--

<?php
  echo $names[$i];
?>

</h3>
<object classid="clsid:d27cdb6e-ae6d-11cf-96b8-444553540000"
        codebase="http://download.macromedia.com/pub/shockwave/cabs/flash/swflash.cab#version=9,0,0,0"
        width="337" height="250" id="demo" align="middle">
  <param name="allowScriptAccess" value="sameDomain" />
  <param name="allowFullScreen" value="false" />
  <param name="movie"

<?php
  echo "value=\"" . $url . "\"";
?>

  />
  <paran name="hostname"

<?php
  echo "value=\"" . $hosts[$i] . "\" ";
?>

  />
  <param name="quality" value="high" />
  <param name="bgcolor" value="#cccccc" />
  <embed

<?php
  echo "src=\"" . $url . "\" ";
  echo "hostname=\"" . $hosts[$i] . "\" ";
?>

quality="high" bgcolor="#cccccc"
         width="337" height="250" name="demo" align="middle"
         allowScriptAccess="sameDomain" allowFullScreen="false"
         type="application/x-shockwave-flash"
         pluginspage="http://www.macromedia.com/go/getflashplayer" />
</object>
</td>

<?php
}

?>

</tr>
</table>
</body>
</html>
