<?php
if ($_SERVER["QUERY_STRING"]) {
    $query_string = $_SERVER["QUERY_STRING"] . "&protogeni=1";
}
else {
    $query_string = "protogeni=1";
}
header("Location: news.php3?${query_string}");
?>

