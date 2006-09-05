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
     - 105 separate forms are on 95 php code pages (plus 7 "extras" on Boss.)

   . Spider a copy of the EinE site with wget and extract its forms list.
       gmake spider
       gmake site_forms
     - 40 "base" forms are visible once logged in as user, 47 with admin on.

   . Compare the two lists to find uncovered (unlinked) forms.
       gmake forms_coverage

   . Create a script to activate the EinE site to turn on all forms.
     - Look in the sources to find where the missing links should be.
     - Connect to the EinE site from a browser through Spike Proxy.
     - Interactively create DB state that will elicit the uncovered forms.
       . Projects/users awaiting approval, 
       . Experiments swapped in with active nodes, and so on.
     - Capture a list of URL's along with Get or Post inputs for automation.
     - Convert the list into an wget script and/or WebInject test cases.

   . Re-spider and compare until everything is covered (no more missing forms.)
       gmake spider
       gmake forms_msg

 - Input fields coverage
   . Grep spidered forms for <input definitions and devise acceptable values.
       gmake input_coverage
     - 1631 <input lines in admin-base, 511 unique, with 156 unique field names.
     - But only 78 of the unique field names are text fields.

   . Convert the list to WebInject XML test cases submitting input field values.
   . Test using WebInject until "normal" input tests work properly in all forms.

 - Probe the checking code of all input fields for SQL injection holes
   . Generate WebInject cases with SQL injection probes in individual fields.
     Probe strings include form and field names that caused the hole.
   . Successfully caught cases should produce "invalid input" warnings.
   . Potential penetrations will log SQL errors with the form/field name.

 - Plug all of the holes by adding or fixing input validation logic.
   . Re-run probes to check.
   . Re-do it periodically, as the system evolves.
