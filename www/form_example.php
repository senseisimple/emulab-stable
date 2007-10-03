<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("form_defs.php");

PAGEHEADER("Silly Forms example");

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();

$optargs = OptionalPageArguments("submit",      PAGEARG_STRING,
				 "formfields",  PAGEARG_ARRAY);
if (!isset($formfields)) {
     $formfields = null;
}

# The form attributes:
$form = array('#id'	  => 'form1',
	      '#caption'  => 'My Form',
	      '#action'   => 'form_example.php');

# A set of form fields.
$fields = array();

# Text field.
$fields['slot1'] = array('#type'        => 'textfield',
			 '#label'       => 'Slot1',
			 '#value'       => 'Hello World',
			 '#size'        => 60,
			 '#maxlength'   => 64,
			 '#description' => 'Alphanumeric, No Blanks'
			 );

# Password
$fields['slot2'] = array('#type'	 => 'password',
			 '#label'        => 'Password',
			 '#required'     => TRUE,
			 '#footnote'     => 'This is a footnote',
			 '#size'         => 8);

# File Upload. You must set '#entype' in the form array.
$fields['myfile'] = array('#type'        => 'file',
			  '#label'       => 'Your File',
			  '#size'        => 30,
			  '#description' => 'An NS File'
			  );

# A plain checkbox
$fields['slot3']  = array('#type'        => 'checkbox',
			  '#return_value'=> "Yep",
			  '#label'       => 'Check this Box',
			  );

# A list of checkboxes
$fields['slot4']  = array('#type'        => 'checkboxes',
			  '#label'       => 'Check some of these',
			  '#boxes'       =>
			    array('box1' => array('#return_value' => "Yep",
						  '#label' => 'L1'
						  ),
				  'box2' => array('#return_value' => "Yep",
						  '#label' => 'L2'
						  ),
				  'box3' => array('#return_value' => "Yep",
						  '#label' => 'L3'
						  ),
				  ),
			  );

# A radio checklist.
$fields['slot5']  = array('#type'       => 'radio',
			  '#label'      => 'Check one of these',
			  '#radios'     =>
			   array('Rad1' => array('#label' => 'R1',
						 '#return_value' => "R1v",
						 ),
				 'Rad2' => array('#label' => 'R2',
						 '#return_value' => "R2v",
						 ),
				 'Rad3' => array('#label' => 'R3',
						 '#return_value' => "R3v",
						 ),
				 ),
			  );

# A selection list.
$fields['slot6']  = array('#type'       => 'select',
			  '#label'      => 'A selection of items',
			  '#default_value' => 'B',
			  '#options'    => array('A' => 'Pick A',
						 'B' => 'Pick B',
						 'C' => 'Pick C'),
			  '#description'=> 'Bla Bla Bla'
			  );

# A hidden Field.
$fields['slot7']  = array('#type'  => 'hidden',
			  '#value' => "69",
			  );

# A simple subsection (rendered as an inner table).
$fields['address'] = array('#type'   => 'table',
			   '#label'  => 'Fill in your addresss',
			   '#fields' =>
			    array('part1' => array('#type' => 'textfield',
						   '#label' => 'Street',
						   '#size' => 60,
						   ),
				  'part2' => array('#type' => 'textfield',
						   '#label' => 'Town',
						   '#value' => 'Corvallis',
						   '#size' => 30,
						   ),
				  ),
			   );
# The submit button.				
$fields['submit']  = array('#type'  => 'submit',
			   '#value' => "Submit This Form",
			   );

echo "<center>";
FormRender($form, null, $fields, $formfields);
echo "</center>";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
