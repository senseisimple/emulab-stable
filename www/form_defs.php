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
#                '#prep'       => 'trim'        # Optional used by FormValidate
#                '#checkslot'  => 'default:text'# Optional used by FormValidate
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
# 5) A note about javascript. Many existing forms use the convention of
# naming them with name='myform' which is now deprecated. The new way of
# doing this is with id='myform' ... but the problem is that name='myform'
# created a global variable called myform that referenced the javascript
# object, while id='myform' does not. This is not so terrible, since the
# changes are simple.
#
# Change:
#	'#javascript' => "onchange=\"SetWikiName(myform);\"",
# to:
# 	'#javascript' => "onchange=\"SetWikiName('myform');\"",
#
# note that myform is in single quotes. In the function definition, change
# the prolog from:
#
#             function SetWikiName(theform) 
#             {
# to:
#             function SetWikiName(formname) 
#             {
#                 var theform = getObjbyName(formname);
#
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
    $return_value = isset($attrs['#return_value']) ? $attrs['#return_value'] : 1;
    $html .= "value=\"" . $return_value . "\" ";
    if (isset($attrs['#value']) &&
	$attrs['#value'] == $return_value) {
	$html .= "checked ";
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
    $html = "<select ";

    if (array_key_exists('#name', $attrs)) {
	$html .= " name='" . $attrs['#name'] . "' ";
    }
    else {
	$html .= " name=\"formfields[$name]\" ";
    }
    if (isset($attrs['#class'])) {
	$html .= "class=\"" . $attrs['#class'] . "\" ";
    }
    else {
	$html .= "class=\"form-select\" ";
    }
    if (isset($attrs['#javascript'])) {
	$html .= $attrs['#javascript'] . " ";
    }
    if (array_key_exists('#id', $attrs)) {
	$html .= " id='" . $attrs['#id'] . "' ";
    }
    $html .= ">\n";

    if (!@$attrs['#no_default']) {
	if (isset($attrs['#default']))
	    $default = $attrs['#default'];
	else
	    $default = "Please Select";
	$html .= "<option value=''>$default &nbsp</option>\n";
    }

    if (isset($attrs['#options'])) {
        $options = $attrs['#options'];
        if (isset($attrs['#value']) && $attrs['#value'] != ''  
            && !isset($options[$attrs['#value']]))
  	    $options[$attrs['#value']] = $attrs['#value'];
	foreach ($options as $selectvalue => $selectlabel) {
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

function FormRenderTextArea($name, $attrs)
{
    $html = "<textarea name=\"formfields[$name]\" ";
    if (isset($attrs['#javascript'])) {
	$html .= $attrs['#javascript'] . " ";
    }
    if (isset($attrs['#class'])) {
	$html .= "class=\"" . $attrs['#class'] . "\" ";
    }
    else {
	$html .= "class=\"form-text\" ";
    }
    if (isset($attrs['#cols'])) {
	$html .= "cols=" . $attrs['#cols'] . " ";
    }
    if (isset($attrs['#rows'])) {
	$html .= "rows=" . $attrs['#rows'] . " ";
    }
    $html .= ">";
    if (isset($attrs['#value'])) {
	$html .= htmlspecialchars($attrs['#value']);
    }
    $html .= "</textarea>\n";
    return $html;
}

function FormRenderSubmit($name, $attrs)
{
    $html = "";

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
    $html .= ">";
    
    return $html;
}

function FormRenderImage($name, $attrs)
{
    $html = "";
    $img  = $attrs['#image'];

    $html .= "<input type=image name=\"formfields[$name]\" ";
    if (isset($attrs['#value'])) {
	$html .= "value=\"" . $attrs['#value'] . "\" ";
    }
    else {
	$html .= "value=Submit ";
    }
    if (isset($attrs['#class'])) {
	$html .= "class=\"" . $attrs['#class'] . "\" ";
    }
    else {
	$html .= "class=\"form-image\" ";
    }
    if (isset($attrs['#javascript'])) {
	$html .= $attrs['#javascript'] . " ";
    }
    $html .= "src=$img ";
    $html .= ">";
    
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

# Render a list of elements together.
function FormRenderList($name, $attributes, $submitted)
{
    $html = "";
    
    while (list ($subname, $subattrs) = each ($attributes['#elements'])) {
	if ($submitted && array_key_exists($subname, $submitted)) {
	    $subattrs['#value'] = $submitted[$subname];
	}
	$html .= FormRenderElement($subname, $subattrs, $submitted);
	if (isset($subattrs['#label']))
	    $html .= $subattrs['#label'] . " &nbsp; ";
	$html .= " &nbsp; ";
    }
    return $html;
}

# Render a list of elements together, vertically
function FormRenderVList($name, $attributes, $submitted)
{
    $html = "";
    
    while (list ($subname, $subattrs) = each ($attributes['#elements'])) {
	if ($submitted && array_key_exists($subname, $submitted)) {
	    $subattrs['#value'] = $submitted[$subname];
	}
	if (isset($subattrs['#label']))
	    $html .= $subattrs['#label'] . ": ";
	$html .= FormRenderElement($subname, $subattrs, $submitted);
	$html .= "<br>";
    }
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
	$field_html .=
	    "<input type=hidden name=\"formfields[$name]\" ".
	    "value=\"$value\">\n";
	break;
    case "submit":
	$field_html = FormRenderSubmit($name, $attributes);
	break;
    case "image":
	$field_html = FormRenderImage($name, $attributes);
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
    case "list":
	$field_html = FormRenderList($name, $attributes, $submitted);
	break;
    case "vlist":
	$field_html = FormRenderVList($name, $attributes, $submitted);
	break;
    case "textarea":
	$field_html = FormRenderTextArea($name, $attributes, $submitted);
	break;
    case "display":
	$field_html = isset($submitted[$name]) ? $submitted[$name] : $attributes['#value'];
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
	    if ($attributes['#type'] == "table" || $attributes['#type'] == "textarea")
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
	    if ($attributes['#type'] != "submit" &&
                isset($attributes['#label']) &&
		!isset($attributes['#colspan'])) {
		$html .= "<td align=left $mouseover $cols>";

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
		} else if ($attributes['#type'] == "textarea") {
	 	    $html .= "<br>$field_html</td>";
                } else {
		    $html .= "</td>";
		    $html .= "<td align=left>$field_html</td>";
		}
	    }
	    else {
		$html .= "<td align=center colspan=2>";
		$html .= "$field_html";
		$html .= "</td>\n";
	    }
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
	reset($errors);
    }
    $html .= "<div align=center>\n";
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
	$html .= "<div style=\"width:90%; margin:auto; text-align: left\">\n";
	$html .= "<font size=-1><ol>\n";
	foreach ($footnotes as $i => $note) {
	    $html .= "<li>$note</li>\n";
	}
	$html .= "</ol></font></div>\n";
    }
    $html .= "</div>\n";
    
    echo "$html\n";
}

#
# FormValidate and helper functions
#

function CombineLabels($parent, $attributes) {
    $res = '';
    $label = @$attributes['#label'];
    $res .= $parent;
    if ($parent != '' && $label != '') $res .= ', ';
    $res .= $label;
    return $res;
}

function FormValidateElement($name, &$errors, $attributes, &$submitted, $parent_label)
{
    $error_label = CombineLabels($parent_label, $attributes);
    # Check for required fields not filled out
    if (isset($attributes['#prep']) && 
	isset($submitted[$name]) && $submitted[$name] != "") {
        $submitted[$name] = $attributes['#prep']($submitted[$name]);
    }
    if (isset($attributes['#required']) && $attributes['#required'] &&
	!(isset($submitted[$name]) && $submitted[$name] != "")) {
	$errors[$error_label] = "Missing required value";
    }
    elseif (isset($attributes['#checkslot']) &&
	    isset($submitted[$name]) && $submitted[$name] != "") {
	$check = $attributes['#checkslot'];
	
	if (function_exists($check)) {
	    $check($name, $errors, $attributes, $submitted[$name]);
	}
	elseif (preg_match("/^([-\w]+):([-\w]+)$/", $check, $matches)) {
	    #
	    # What if not required and not set?
	    #
	    if (!isset($submitted[$name]) || $submitted[$name] == "") {
		$submitted[$name] = "";
	    }
	    if (!TBcheck_dbslot($submitted[$name],
				$matches[1], $matches[2],
				TBDB_CHECKDBSLOT_WARN|
				TBDB_CHECKDBSLOT_ERROR)) {
		$errors[$error_label] = TBFieldErrorString();
	    }
	}
	elseif (substr($check, 0, 1) == "/") {
	    # Regular expression.
	    if (!preg_match($check, $submitted[$name])) {
		$errors[$error_label] = "Illegal characters";
	    }
	}
	else {
	    TBERROR("Could not parse checkslot: $check", 1);
	}
    }
}

function FormValidateFileUpload($name, &$errors, $attributes)
{
    $error_label = CombineLabels($parent_label, $attributes);
    # Check for required fields not filled out
    if (isset($attributes['#required']) && $attributes['#required'] &&
	!(isset($_FILES[$name]['name']) && $_FILES[$name]['size'] != 0)) {
	$errors[$error_label] = "Missing required value";
    }
    elseif (isset($attributes['#checkslot'])) {
	$check = $attributes['#checkslot'];
	
	if (function_exists($check)) {
	    $check($name, $errors, $attributes, null);
	}
	else {
	    TBERROR("Could not parse checkslot: $check", 1);
	}
    }
}

function FormValidate($form, &$errors, $fields, &$submitted, $parent_label = '') 
{
    while (list ($name, $attributes) = each ($fields)) {
	switch ($attributes['#type']) {
	case "textfield":
	case "password":
	case "hidden":
	case "submit":
	case "checkbox":
	case "radio":
	case "select":
        case "textarea":
	    FormValidateElement($name, $errors, $attributes, $submitted, $parent_label);
	    break;
	case "checkboxes":
	    while (list ($subname, $subattrs) = each ($attributes['#boxes'])) {
		FormValidateElement($subname, $errors, $subattrs, $submitted,
		                    CombineLabels($parent_label, $attributes));
	    }
	    break;
	case "table":
	    FormValidate($form, $errors, $attributes['#fields'], $submitted,
                         CombineLabels($parent_label, $attributes));
	    break;
	case "list":
        case "vlist":
	    while (list ($subname, $subattrs) =
		   each ($attributes['#elements'])) {
		FormValidateElement($subname, $errors, $subattrs, $submitted,
                                    CombineLabels($parent_label, $attributes));
	    }
	    break;
	case "file":
	    FormValidateFileUpload($name, $errors, $attributes, $parent_label);
	    break;
        case "display":
            break;
	default:
	    $errors[$name] = "Invalid slot type: " . $attributes['#type'];
	    break;
	}
    }
}

#
# FormTextDump and helper functions
#

function FormTextDumpElement($name, $attributes, $values, $label_width, $parent_label)
{
    $res = '';
    if (!isset($values[$name]) || $values[$name] == '' || @$attributes['#nodump'])
        return $res;
    $label = CombineLabels($parent_label, $attributes);
    $type = $attributes['#type'];
    $value = $values[$name];
    if ($type == 'checkbox')
        $value = $values[$name] ? 'true' : 'false';
    if ($type == 'select' && isset($attributes['#options'][$value]))
        $value = $attributes['#options'][$value];
    if ($type == 'textarea') {
      $res .= "$label: \n";
      $res .= $value;
    } else {
      $res .= sprintf("%-${label_width}s %s\n", "$label:", $value);
    }
    return $res;
}

function FormTextDump($form, $fields, $values, $label_width = 20, $parent_label = '') 
{
    $res = '';
    while (list ($name, $attributes) = each ($fields)) {
	switch ($attributes['#type']) {
	case "hidden":
	case "textfield":
	case "password":
	case "submit":
	case "checkbox":
	case "radio":
	case "select":
	case "textarea":
        case "display":
	    $res .= FormTextDumpElement($name, $attributes, $values, 
                                        $label_width, $parent_label);
	    break;
	case "checkboxes":
	    while (list ($subname, $subattrs) = each ($attributes['#boxes'])) {
		FormTextDumpElement($subname, $subattrs, $values, $label_width,
		                    CombineLabels($parent_label, $attributes));
	    }
	    break;
	case "table":
	    $res .= FormTextDump($form, $attributes['#fields'], $values, $label_width,
                                 CombineLabels($parent_label, $attributes));
	    break;
	case "list":
	case "vlist":
	    while (list ($subname, $subattrs) =
		   each ($attributes['#elements'])) {
		$res .= FormTextDumpElement($subname, $subattrs, $values, $label_width,
                                            CombineLabels($parent_label, $attributes));
	    }
	    break;
	case "file":
	    # Skip for now
	    break;
	default:
	    user_error("Invalid slot type \"". $attributes['#type'] ."\" in FormTextDump",
                       E_USER_NOTICE);
	    break;
	}
    }
    return $res;
}
?>