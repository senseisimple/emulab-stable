<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005-2011 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

$optargs = OptionalPageArguments("protogeni", PAGEARG_BOOLEAN);

if (! isset($protogeni) || !$protogeni) {
    $protogeni = 0;
}
else {
    $protogeni = 1;
}
$db_table = ($protogeni ? "webnews_protogeni" : "webnews");

header("Content-type: text/xml");

$query_result=
    DBQueryFatal("SELECT subject, author, body, msgid, ".
    		 "date, usr_name " .
		 "FROM $db_table as w ".
                 "LEFT JOIN users on w.author_idx = users.uid_idx " .
		 "ORDER BY w.date DESC " .
                 "LIMIT 5");

?>
<rss version="2.0"> <channel>
    <title><? echo $THISHOMEBASE ?> News</title>
<?
echo "<link>$TBBASE/news.php3?protogeni=$protogeni</link>\n";
?>
    <description>News items for <? echo $THISHOMEBASE ?></description>
    <docs>http://blogs.law.harvard.edu/tech/rss</docs>
    <managingEditor><? echo $TBMAILADDR_OPS ?></managingEditor>
    <webMaster><? echo $TBMAILADDR_OPS ?></webMaster>
    <pubDate><? echo date("r"); ?></pubDate>
<?

$first = 1;
while ($row = mysql_fetch_array($query_result)) {
    $subject     = $row["subject"];
    $timestamp   = $row['date'];
    $author      = $row["author"];
    $author_name = $row["usr_name"];
    $body        = $row["body"];
    $msgid       = $row["msgid"];

    # Strip HTML from the body
    $stripped = strip_tags($body);

    # Try to grab just the first two sentences of the body
    preg_match("/^(([^.]+\.){0,2})\s*(.*)/",$stripped,$matches);
    if ($matches[1]) {
        $summary = $matches[1];
        if ($matches[3] != "") {
            $summary .= " ... ";
        }
    } else {
        $summary = $stripped;
    }

    # Have to convert the date/time to RFC822 format
    list($date, $hours) = preg_split('/ /', $timestamp);
    list($year,$month,$day) = preg_split('/-/',$date);
    list($hour,$min,$sec) = preg_split('/:/',$hours);
    $rfc822date = date("r",mktime($hour, $min, $sec, $month, $day, $year));

    if ($first) {
        # If this is the 'first' article (the most recent), include it as
        # the date the channel was last updated
        echo "    <lastBuildDate>" . $rfc822date . "</lastBuildDate>\n";
        $first = 0;
    }
    if ($protogeni) {
	$url = "$TBBASE/pgeninews.php?single=$msgid";
    }
    else {
	$url = "$TBBASE/news.php3?single=$msgid";
    }

    echo "    <item>\n";
    echo "        <title>$subject</title>\n";
    echo "        <link>$url</link>\n";
    echo "        <guid isPermaLink=\"true\">$url</guid>\n";
    echo "        <description>$summary</description>\n";
    echo "        <pubDate>$rfc822date</pubDate>\n";
    echo "        <author>$author_name</author>\n";
    echo "    </item>\n";
}
?>
</channel> </rss>
