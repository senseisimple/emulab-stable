<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
$footnotes = null;

# The idea behind this is simplify and make all of the form construction
# coherrent across the entire web interface. Steps to creating a form:
#
# 1) Create an array of attributes for the form.
#
#   $form = array('#id'	      => 'myform',	# Optional; default: myform
#	  	  '#action'   => 'foo.php',       # Required
#		  '#method'   => 'POST',          # Optional; default POST
#		  '#class'    => 'standard',      # Required; default standard
#		  '#caption'  => 'A Form',        # Optional; default ''
#		  '#enctype'  => '');             # Optional
#
# 2) Create an array of form fields:
#
#     $fields = array();
#     $fields['Name'] =
#          array('#type'       => 'textfield',	# Required
#		 '#label'      => 'Full Name',  # Required
#		 '#description'=> 'Your Name',  # Optional (extra text)
#                '#link'       => 'foo.html',   # Optional (link for label).
#		 '#size'       => 30,           # Optional
#		 '#maxlength'  => 30,           # Optional
#		 '#required'   => TRUE,         # Optional
#                '#mouseover'  => 'Mouse Me',   # Optional string over label
#		 '#footnote'   => 'See ...',    # Optional footnote section.
#		 '#javascript' => "...");       # Optional
#
# 3) Call FormRender with the above, plus optional $errors and $values
#    arrays. Both of these are exactly as we use them now:
#
#	FormRender($form, $fields, $errors, $values);
#
#   which spits the html to the output stream.
#
# 4) The 'action' page will get $submit and $formfields as input.
#
# * See form_example.php for a good example usage.
#
function FormRenderTextField($name, $attrs)
{
    $type = ($attrs['#type'] == "password" ? "password" : "text");
	
    $html = "<input type=$type name=\"formfields[$name]\" ";
    if (isset($attrs['#value'])) {
	$html .= "value=\"" . $attrs['#value'] . "\" ";
    }
    if (isset($attrs['#javascript'])) {
	$html .= $attrs['#javascript'] . " ";
    }
    if (isset($attrs['#class'])) {
	$html .= "class=\"" . $attrs['#class'] . "\" ";
    }
    else {
	$html .= "class=\"form-text\" ";
    }
    if (isset($attrs['#size'])) {
	$html .= "size=" . $attrs['#size'] . " ";
    }
    if (isset($attrs['#maxlength'])) {
	$html .= "maxlength=" . $attrs['#maxlength'] . " ";
    }
    $html .= ">";
    
    return $html;
}

function FormRenderCheckBox($name, $attrs)
{
    $html = "<input type=checkbox name=\"formfields[$name]\" ";
    if (isset($attrs['#return_value'])) {
	$html .= "value=\"" . $attrs['#return_value'] . "\" ";
	if (isset($attrs['#value']) &&
	    $attrs['#value'] == $attrs['#return_value']) {
	    $html .= "checked ";
	}
    }
    if (isset($attrs['#class'])) {
	$html .= "class=\"" . $attrs['#class'] . "\" ";
    }
    else {
	$html .= "class=\"form-checkbox\" ";
    }
    if (isset($attrs['#javascript'])) {
	$html .= $attrs['#javascript'] . " ";
    }
    $html .= ">";
    
    return $html;
}

function FormRenderRadio($name, $attrs)
{
    $html = "";
    
    while (list ($subname, $subattrs) = each ($attrs['#radios'])) {
	$html .= "<input type=radio name=\"formfields[$name]\" ";
	if (isset($attrs['#class'])) {
	    $html .= "class=\"" . $attrs['#class'] . "\" ";
	}
	else {
	    $html .= "class=\"form-radio\" ";
	}
	if (isset($subattrs['#return_value'])) {
	    $html .= "value=\"" . $subattrs['#return_value'] . "\" ";
	    if (isset($attrs['#value']) &&
		$attrs['#value'] == $subattrs['#return_value']) {
		$html .= "checked ";
	    }
	}
	if (isset($attrs['#javascript'])) {
	    $html .= $attrs['#javascript'] . " ";
	}
	$html .= ">";
	if (isset($subattrs['#label']))
	    $html .= $subattrs['#label'] . " &nbsp; ";
	$html .= "\n";
    }
    return $html;
}

function FormRenderSelect($name, $attrs)
{
    $html = "<select name=\"formfields[$name]\" ";
    if (isset($attrs['#class'])) {
	$html .= "class=\"" . $attrs['#class'] . "\" ";
    }
    else {
	$html .= "class=\"form-select\" ";
    }
    if (isset($attrs['#javascript'])) {
	$html .= $attrs['#javascript'] . " ";
    }
    $html .= ">\n";
    $html .= "<option value=''>Please Select &nbsp</option>\n";

    if (isset($attrs['#options'])) {
	while (list ($selectvalue, $selectlabel) = each ($attrs['#options'])) {
	    $selected = "";	    
	    if (isset($attrs['#value']) && $attrs['#value'] == $selectvalue) {
		$selected = "selected";
	    }
	    $html .= "<option $selected value='$selectvalue'>";
	    $html .= "$selectlabel </option>\n";
	}
    }
    $html .= "</select>\n";
    return $html;
}

function FormRenderFile($name, $attrs)
{
    $html = "<input type=file name=\"$name\" ";
    if (isset($attrs['#value'])) {
	$html .= "value=\"" . $attrs['#value'] . "\" ";
    }
    if (isset($attrs['#javascript'])) {
	$html .= $attrs['#javascript'] . " ";
    }
    if (isset($attrs['#class'])) {
	$html .= "class=\"" . $attrs['#class'] . "\" ";
    }
    else {
	$html .= "class=\"form-file\" ";
    }
    if (isset($attrs['#size'])) {
	$html .= "size=" . $attrs['#size'] . " ";
    }
    $html .= ">";
    
    return $html;
}

function FormRenderSubmit($name, $attrs)
{
    $html = "";

    $html .= "<td align=center colspan=2>";
    $html .= "<input type=submit name=\"$name\" ";
    if (isset($attrs['#value'])) {
	$html .= "value=\"" . $attrs['#value'] . "\" ";
    }
    else {
	$html .= "value=Submit";
    }
    if (isset($attrs['#class'])) {
	$html .= "class=\"" . $attrs['#class'] . "\" ";
    }
    else {
	$html .= "class=\"form-submit\" ";
    }
    if (isset($attrs['#javascript'])) {
	$html .= $attrs['#javascript'] . " ";
    }
    $html .= "></td>";
    
    return $html;
}

# Render a subsection (table) of a form.
function FormRenderTable($name, $attributes, $submitted)
{
    $html = "<table align=center border=1>\n";
    $html .= FormRenderElements($attributes['#fields'], $submitted);
    $html .= "</table>\n";
    return $html;
}

function FormRenderElement($name, $attributes, $submitted)
{
    $field_html = null;

    #
    # The value that was submitted overrides the value in the attributes.
    # For most things, we just munge the attributes field.
    #
    if ($submitted && array_key_exists($name, $submitted)) {
	$attributes['#value'] = $submitted[$name];
    }
    
    switch ($attributes['#type']) {
    case "textfield":
    case "password":
	$field_html = FormRenderTextField($name, $attributes);
	break;
    case "hidden":
	$value = $attributes['#value'];
	$field_html .= "<input type=hidden name=$name value=\"$value\">\n";
	break;
    case "submit":
	$field_html = FormRenderSubmit($name, $attributes);
	break;
    case "checkbox":
	$field_html = FormRenderCheckBox($name, $attributes);
	break;
    case "radio":
	$field_html = FormRenderRadio($name, $attributes);
	break;
    case "file":
	$field_html = FormRenderFile($name, $attributes);
	break;
    case "checkboxes":
	while (list ($subname, $subattrs) = each ($attributes['#boxes'])) {
	    if ($submitted && array_key_exists($subname, $submitted)) {
		$subattrs['#value'] = $submitted[$subname];
	    }
	    $field_html .= FormRenderCheckBox($subname, $subattrs);
	    if (isset($subattrs['#label']))
		$field_html .= $subattrs['#label'] . " &nbsp; ";
	}
	break;
    case "select":
	$field_html = FormRenderSelect($name, $attributes);
	break;
    case "table":
	$field_html = FormRenderTable($name, $attributes, $submitted);
	break;
    }
    return $field_html;
}

function FormRenderElements($fields, $submitted)
{
    global $footnotes;
    $html = "";
    
    while (list ($name, $attributes) = each ($fields)) {
	$field_html = FormRenderElement($name, $attributes, $submitted);

	if ($field_html) {
	    if ($attributes['#type'] == "hidden") {
		$html .= $field_html;
		continue;
	    }
	    $cols  = "";
	    if ($attributes['#type'] == "table")
		$cols = "colspan=2";

	    $mouseover = "";
	    if (isset($attributes['#mouseover'])) {
		$mouseover = FormRenderMouseOver($attributes['#mouseover']);
	    }

	    # Record footnote for later.
	    $footnote = "";
	    if (isset($attributes['#footnote'])) {
		$thisnote = 0;
		
		#
		# Slight complication; we want to be able to specify the
		# same footnote multiple times, but show it only once.
		#
		foreach ($footnotes as $i => $note) {
		    if ($note == $attributes['#footnote']) {
			$thisnote = $i + 1;
			break;
		    }
		}
		if (! $thisnote) {
		    $thisnote = count($footnotes) + 1;
		    $footnotes[] = $attributes['#footnote'];
		}
		$footnote = "[<b>$thisnote</b>]";
	    }
		
	    $html .= "<tr>";
	    if ($attributes['#type'] != "submit") {
		$html .= "<td $mouseover $cols>";

		# Required fields mark with *
		if (isset($attributes['#required']) &&
		    $attributes['#required'])
		    $html .= "*";

		$label = $attributes['#label'];
		if (isset($attributes['#link']))
		    $label = "<a href=\"" . $attributes['#link'] . "\"" .
			"target=_blank>$label</a>";
		$html .= $label . $footnote . ": ";
		if (isset($attributes['#description']))
		    $html .= "<br><font size=-2>(" .
			$attributes['#description'] . ")";
		if ($attributes['#type'] == "table") {
		    $html .= "$field_html</td>";
		}
		else {
		    $html .= "</td>";
		    $html .= "<td>$field_html</td>";
		}
	    }
	    else 
		$html .= "$field_html";
	    $html .= "</tr>\n";
	}
    }
    return $html;
}

function FormRenderMouseOver($string)
{
    $string = ereg_replace("\n", "<br>", $string);
    $string = ereg_replace("\r", "", $string);
    $string = htmlentities($string);
    $string = preg_replace("/\'/", "\&\#039;", $string);

    return "onmouseover=\"return escape('$string')\"";
}

function FormRender($attributes, $errors, $fields, $submitted = null)
{
    global $footnotes;
    
    $html = "";
    $footnotes = array();

    $action = $attributes['#action'];
    $id = "myform";
    if (array_key_exists('#id', $attributes))
	$id = $attributes['#id'];
    $caption = null;
    if (array_key_exists('#caption', $attributes))
	$caption = $attributes['#caption'];
    $method = "POST";
    if (array_key_exists('#method', $attributes))
	$method = $attributes['#method'];
    $enctype = "";
    if (array_key_exists('#enctype', $attributes))
	$enctype = "enctype=" . $attributes['#enctype'];
    $class = "standard";
    if (array_key_exists('#class', $attributes))
	$class = $attributes['#class'];

    if ($errors) {
	$html .= "<table class=nogrid border=0 cellpadding=6 cellspacing=0>";
	$html .= "<tr><th align=center colspan=2>";
	$html .= "<font size=+1 color=red>";
	$html .= "&nbsp;Oops, please fix the following errors!&nbsp;";
	$html .= "</font></td></tr>\n";
	
	while (list ($name, $message) = each ($errors)) {
	    $html .= "<tr><td align=right>";
	    $html .= "<font color=red>$name:&nbsp;</font></td>";
	    $html .= "<td align=left><font color=red>$message</font></td>";
	    $html .= "</tr>\n";
	}
	$html .= "</table><br><br>\n";
    }

    $html .= "<table align=center border=1>\n";
    if ($caption)
	$html .= "<caption>$caption</caption>\n";
    $html .= "<tr><td align=center colspan=2>";
    $html .= "<em>(Fields marked with * are required)</em></td></tr>";

    $html .= "<form action='$action' $enctype method=$method id=$id ";
    $html .= "class='$class'>";
    $html .= FormRenderElements($fields, $submitted);
    $html .= "</form>";
    $html .= "</table>\n";

    if (count($footnotes)) {
	$html .= "<div align=left><h4><ol>\n";
	foreach ($footnotes as $i => $note) {
	    $html .= "<li>$note\n";
	}
	$html .= "</ol></h4></div>\n";
    }
    
    echo "$html\n";
}

function FormValidate($form, &$errors, $fields, $submitted)
{
    while (list ($name, $attributes) = each ($fields)) {
        # Check for required fields not filled out
	if (isset($attributes['#required']) && $attributes['#required'] &&
	    !(isset($submitted[$name]) && $submitted[$name] != "")) {
	    $errors[$attributes['#label']] = "Missing required value";
	}
	else if (isset($attributes['#checkslot']) &&
		 isset($submitted[$name]) && $submitted[$name] != "") {
	    # Check slot
	    if (preg_match("/^([-\w]+):([-\w]+)$/",
			   $attributes['#checkslot'], $matches)) {

		if (!TBcheck_dbslot($submitted[$name],
				    $matches[1], $matches[2],
				    TBDB_CHECKDBSLOT_WARN|
				    TBDB_CHECKDBSLOT_ERROR)) {
		    $errors[$attributes['#label']] = TBFieldErrorString();
		}
	    }
	    else {
		TBERROR("Could not parse " . $attributes['#checkslot'], 1);
	    }
	}
    }
}
?>
