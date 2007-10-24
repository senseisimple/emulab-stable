<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
$footnotes = null;

function TableRender($attributes, $rows)
{
    global $footnotes;
    
    $html   = "";
    $button = "";

    if (array_key_exists('#footnotes', $attributes)) {
	$footnotes = $attributes['#footnotes'];
    }
    else {
	$footnotes = array();
    }

    $id = "mytable";
    if (array_key_exists('#id', $attributes))
	$id = $attributes['#id'];
    $caption = null;
    if (array_key_exists('#caption', $attributes))
	$caption = $attributes['#caption'];
    $title = null;
    if (array_key_exists('#title', $attributes))
	$title = $attributes['#title'];
    $class = "standard";
    if (array_key_exists('#class', $attributes))
	$class = $attributes['#class'];
    $align = "center";
    if (array_key_exists('#align', $attributes))
	$align = $attributes['#align'];
    $sortable = "";
    if (array_key_exists('#sortable', $attributes))
	$sortable = "class=sort";
    $visclass = "";
    $checked  = "checked=checked";
    if (array_key_exists('#invisible', $attributes) &&
	$attributes['#invisible']) {
	$visclass = "class=invisible";
	$checked  = "";
    }

    $button = "<input type=checkbox $checked id=\"${id}_checkbox\" ".
	"onclick='return toggle_table(\"${id}\", ".
	"\"${id}_checkbox\", \"${id}_title\", \"${id}_footnotes\");'>";

    if ($caption) {
	$html .= "<table align=center class=stealth>";
	$html .= "<tr><td class=stealth>$button</td>";
	$html .= "<td class=stealth>".
	    "<font size=+1><b>$caption</b></font></td>\n";
	$html .= "</tr></table>";
    }
    else if ($title) {
	$html .= "<span $visclass id='${id}_title'>";
	$html .= "<br><center><font size=+1><b>$title</b></font></center><br>";
	$html .= "</span>\n";
    }
    
    $html .= "<table $visclass ".
	"align=$align border=1 cellpadding=1 cellspacing=2 id='$id'>\n";

    if (array_key_exists('#headings', $attributes)) {
	$html .= "<thead $sortable'>";
	$html .= "<tr>";
	while (list($key, $heading) = each($attributes['#headings'])) {
	    $html .= "<th>$heading</th>";
	}
	$html .= "</tr>\n";
	$html .= "</thead>\n";
    }
    $html .= "<tbody id='${id}_body'>";
    foreach ($rows as $i => $row) {
	$html .= "<tr>";
	#$key = $row[$attributes['#key']];
	#$html .= "<td>$key</td>";
	if (!array_key_exists('#headings', $attributes) && count($row) == 1) {
	    list($key, $text) = each($row);
	    $html .= "<td>${key}:</td><td>$text</td>";
	}
	else {
	    while (list($key, $text) = each($row)) {
		$html .= "<td>$text</td>";
	    }
	}
	$html .= "</tr>";
    }
    $html .= "</tbody>\n";
    $html .= "</table>\n";
    
    if (count($footnotes)) {
	$html .= "<div $visclass id=\"${id}_footnotes\" ".
	    "align=center ><font size=-1><ol>\n";
	foreach ($footnotes as $i => $note) {
	    $html .= "<li align=left>$note\n";
	}
	$html .= "</ol></font></div>\n";
    }

    if (array_key_exists('#sortable', $attributes)) {
        $html .= "<script type='text/javascript' language='javascript'>
	       sorttable.makeSortable(getObjbyName(\"$id\"));
             </script>\n";
    }
    return array($html, $button);
}

#
# Take a string comprising a table and turn it into a div and a button
# to make it invisible.
#
function TableWrapUp($table_html, $asradio = FALSE, $checked = FALSE,
		     $tableid = null, $buttonid = null, $class = null)
{
    $html = "";
    $button = "";

    if (! $tableid)
	$tableid = "mytable_table";
    if (! $buttonid)
	$buttonid = "mytable_button";
    if (! $class)
	$class = "mytable";
    $checked_token = ($checked ? "checked" : "");
    $visclass      = ($checked ? "" : "class=invisible");
    $type          = ($asradio ? "radio" : "checkbox");
    $name          = ($asradio ? "name=\"$asradio\"" : "");

    $button = "<input type=$type $checked_token id=\"${buttonid}\" $name ".
	"onclick='return toggle_table(\"${tableid}\", \"${buttonid}\");'>";
    $html = "<div $visclass id=\"${tableid}\">$table_html</div>";

    return array($html, $button);
}
