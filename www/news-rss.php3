<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

header("Content-type: text/xml");

$query_result=
    DBQueryFatal("SELECT subject, author, body, msgid, ".
    		 "date, usr_name " .
		 "FROM webnews ".
                 "LEFT JOIN users on webnews.author = users.uid " .
                 "WHERE archived=0 " .
		 "ORDER BY date DESC " .
                 "LIMIT 5");

?>
<rss version="2.0"> <channel>
    <title><? echo $THISHOMEBASE ?> News</title>
    <link><? echo $TBBASE?>/news.php3</link>
    <description>News items for <? echo $THISHOMEBASE ?></description>
    <docs>http://blogs.law.harvard.edu/tech/rss</docs>
    <managingEditor><? echo $TBMAILADDR_OPS ?></managingEditor>
    <webMaster><? echo $TBMAILADDR_OPS ?></webMaster>
    <pubDate><? echo date("r"); ?></pubDate>
<?

$first = 1;
while ($row = mysql_fetch_array($query_result)) {
    $subject     = $row[subject];
    $timestamp   = $row['date'];
    $author      = $row[author];
    $author_name = $row[usr_name];
    $body        = $row[body];
    $msgid       = $row[msgid];

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
    list($date, $hours) = split(' ', $timestamp);
    list($year,$month,$day) = split('-',$date);
    list($hour,$min,$sec) = split(':',$hours);
    $rfc822date = date(r,mktime($hour, $min, $sec, $month, $day, $year));

    if ($first) {
        # If this is the 'first' article (the most recent), include it as
        # the date the channel was last updated
        echo "    <lastBuildDate>" . $rfc822date . "</lastBuildDate>\n";
        $first = 0;
    }

    echo "    <item>\n";
    echo "        <title>$subject</title>\n";
    echo "        <link>$TBBASE/news.php3?single=$msgid</link>\n";
    echo "        <guid isPermaLink=\"true\">$TBBASE/news.php3?single=$msgid</guid>\n";
    echo "        <description>$summary</description>\n";
    echo "        <pubDate>$rfc822date</pubDate>\n";
    echo "        <author>$author_name</author>\n";
    echo "    </item>\n";
}
?>
</channel> </rss>
