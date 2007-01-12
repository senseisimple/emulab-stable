sec-check/README-howto.txt - Documentation outline.

 - Overview
   . Purpose: Locate and plug all SQL injection holes in the Emulab web pages.
     - Guide plugging them and find any new ones we introduce.
   . Method: Combine white-box and black-box testing, with automation.

 - Background
   . Ref "The OWASP Top Ten Project"
      http://www.owasp.org/index.php/Category:OWASP_Top_Ten_Project
     - "The OWASP Top Ten represents a broad consensus about what the most
       critical web application security flaws are."

     - The first flaw on the list (many others are consequences of this one.)
         "A1 Unvalidated Input -
          Information from web requests is not validated before being used by a
	  web application. Attackers can use these flaws to attack backend
	  components through a web application."
	 http://www.owasp.org/index.php/Unvalidated_Input

     - One of the consequences:
         "A6 Injection Flaws -
	  Web applications pass parameters when they access external systems
	  or the local operating system. If an attacker can embed malicious
	  commands in these parameters, the external system may execute those
          commands on behalf of the web application." 
	 http://www.owasp.org/index.php/Injection_Flaws

     - More details:
       . The OWASP Guide Project
       	  http://www.owasp.org/index.php/Category:OWASP_Guide_Project
       . Guide Table of Contents
	  http://www.owasp.org/index.php/Guide_Table_of_Contents
	 - Data Validation
	    http://www.owasp.org/index.php/Data_Validation
	   . Data Validation Strategies
	      http://www.owasp.org/index.php/Data_Validation#Data_Validation_Strategies
	   . Prevent parameter tampering
	      http://www.owasp.org/index.php/Data_Validation#Prevent_parameter_tampering
	   . Hidden fields
	      http://www.owasp.org/index.php/Data_Validation#Hidden_fields
	 - Interpreter Injection
	    http://www.owasp.org/index.php/Interpreter_Injection
	   . SQL Injection
	      http://www.owasp.org/index.php/Interpreter_Injection#SQL_Injection

 - Forms coverage
   . Grep the sources for <form and make up a list of php form files.
       gmake src_forms
         Creates: src_forms.list, src_files.list
     - 105 separate forms are on 95 php code pages (plus 7 "extras" on Boss.)
       gmake src_msg

   . Spider a copy of the EinE site with wget and extract its forms list.
     Have to edit the EinE experiment details into the makefile.
     It's better to change your password in the EinE than put it in the makefile.
     See GNUmakefile.in for details.  
       gmake login
       gmake spider
       gmake site_forms
         Creates: admin.wget subdir, site_forms.list, site_files.list
     - 40 "base" forms are visible once logged in as user, 47 with admin on.
       gmake site_msg

   . Compare the two lists to find uncovered (unlinked) forms.
       gmake forms_coverage
         Creates: files_missing.list
       gmake forms_msg

   . Create a script to activate the EinE site to turn on all forms.
     - Look in the sources to find where the missing links should be.
     - Connect to the EinE site from a browser through Spike Proxy.
     - Interactively create DB state that will elicit the uncovered forms.
       . Projects/users awaiting approval, 
       . Experiments swapped in with active nodes, and so on.
     - Capture a list of URL's along with Get or Post inputs for automation.
     - Add steps to the activate: list in the GNUmakefile.in .

   . Re-spider and compare until everything is covered (no more missing forms.)
       gmake spider
       gmake forms_msg

 - Input fields coverage
   . Grep spidered forms for <input definitions and devise acceptable values.
       gmake input_coverage
         Creates: site_inputs.list, input_names.list
	 You make: input_values.list
      At first, Copy input_names.list to input_values.list,
      then edit default values onto the lines for auto-form-fill-in.
      Values with a leading "!" over-ride an action= arg in the form page URL.
      After the first time, you can merge new ones into input_values.list .
      Lines with no value are ignored and may be flushed if you want.

     - 1631 <input lines in admin-base, 511 unique, with 156 unique field names.
         gmake input_msg
     - But only 78 of the unique field names are text fields.

 - "normal operation" test cases
   . Convert the list to test cases submitting input field values.
       gmake gen_normal
         Creates: site_normal.urls, normal_cases.xml
   . Test until "normal" input tests work properly in all forms.
       gmake run_normal
         Creates: normal_output.xml

 - Probe the checking code of all input fields for SQL injection holes
   . Generate test cases with SQL injection probes in individual fields.
     Probe strings include form and field names that caused the hole.
   . Successfully caught cases should produce "invalid input" warnings.
   . Potential penetrations will log DBQuery errors with the form/field names.

 - Plug all of the holes by adding or fixing input validation logic.
   . Re-run probes to check.
   . Re-do it periodically, as the system evolves.
