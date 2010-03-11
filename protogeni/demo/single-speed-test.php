<html>
<head>
<script type="text/JavaScript">
<!--
  function init(time)
  {
    setTimeout("refresh();", time);
  }

  function refresh()
  {
    if (window.success === undefined) {
      location.reload(true);
    }
  }

  function loadSuccess()
  {
    window.success = 1;
  }

//   -->
</script>
</head>
<body bgcolor="#cccccc" onload="javascript:init(10000);">
<center>
<?php

$name = $_GET["name"];
$cm = $_GET["cm"];
$host = $_GET["hostname"];

$width = 3;

?>

<h3>

<?php
  echo $name;
?>

</h3>
<object classid="clsid:d27cdb6e-ae6d-11cf-96b8-444553540000"
        codebase="http://download.macromedia.com/pub/shockwave/cabs/flash/swflash.cab#version=9,0,0,0"
        width="337" height="250" id="demo" align="middle">
  <param name="allowScriptAccess" value="always" />
  <param name="allowNeworking" value="all" />
  <param name="allowFullScreen" value="false" />
  <param name="movie"
<?php
  echo "value=\"http://" . $host . "/speed-test.swf\"";
?>
 />
  <param name="FlashVars"

<?php
  echo "value=\"url=http://" . $host . "\" ";
?>

  />
  <param name="quality" value="high" />
  <param name="bgcolor" value="#cccccc" />
  <embed
<?php
  echo "src=\"http://" . $host . "/speed-test.swf\"";
?>

<?php
  echo "FlashVars=\"url=http://" . $host . "\" ";
?>

quality="high" bgcolor="#cccccc"
         width="337" height="250" name="demo" align="middle"
         allowScriptAccess="always" allowFullScreen="false"
         allowNetworking="all"
         type="application/x-shockwave-flash"
         pluginspage="http://www.macromedia.com/go/getflashplayer" />
</object>
</center>
</body>
</html>
