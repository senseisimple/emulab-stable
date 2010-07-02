#
# EMULAB-COPYRIGHT
# Copyright (c) 2007 University of Utah and the Flux Group.
# All rights reserved.
#
sec-check/README-background.txt

See README-FIRST.txt for a top-level overall outline.


 - Background on SQL Injection vulnerabilities: Ref "The OWASP Top Ten Project"
     http://www.owasp.org/index.php/Category:OWASP_Top_Ten_Project

   . "The OWASP Top Ten represents a broad consensus about what the most
     critical web application security flaws are."

   . The first flaw on the list (many others are consequences of this one.)
       "A1 Unvalidated Input -
	Information from web requests is not validated before being used by a
	web application. Attackers can use these flaws to attack backend
	components through a web application."
       http://www.owasp.org/index.php/Unvalidated_Input

   . One of the consequences:
       "A6 Injection Flaws -
	Web applications pass parameters when they access external systems
	or the local operating system. If an attacker can embed malicious
	commands in these parameters, the external system may execute those
	commands on behalf of the web application." 
       http://www.owasp.org/index.php/Injection_Flaws

   . More details:
     - The OWASP Guide Project
	 http://www.owasp.org/index.php/Category:OWASP_Guide_Project

     - Guide Table of Contents
	http://www.owasp.org/index.php/Guide_Table_of_Contents

       . Data Validation
	   http://www.owasp.org/index.php/Data_Validation
	 - Data Validation Strategies
	     http://www.owasp.org/index.php/Data_Validation#Data_Validation_Strategies
	 - Prevent parameter tampering
	     http://www.owasp.org/index.php/Data_Validation#Prevent_parameter_tampering
	 - Hidden fields
	     http://www.owasp.org/index.php/Data_Validation#Hidden_fields

       . Interpreter Injection
	   http://www.owasp.org/index.php/Interpreter_Injection
	 - SQL Injection
	     http://www.owasp.org/index.php/Interpreter_Injection#SQL_Injection


 - Automated vulnerability scan tools, search and conclusions

   . In July 2006, I surveyed 29 available free and commercial tools,
     categorized as site mappers, scanners, http hacking tools, proxies,
     exploits, and testing tools.  9 of them were worth a second look.

     Many were Windows-only, or manual tools for "penetration testing" to find
     a single unchecked query hole and then attack a database through it.
     Some had automation specific to Microsoft SQL server, which is easy
     because it reports SQL error messages through query results that can leak
     onto HTML pages.  These can easily be used to locate injection holes and
     spill the internal details of the database schema; then the DB data is
     siphoned out and/or mischief is done through the hole.

     None of the tools targetted MySQL, but I verified that MySQL is still
     vulnerable to SQL injection attack from any unchecked or unescaped inputs
     to GET or POST forms.  Trivially, just include an unmatched single-quote
     in any input string that goes into a dynamically-built SQL query that has
     argument strings delimited by single-quotes.

     This is of course only useful if you have another way to know the
     database schema.  Such as an open-source distribution of our software,
     which would also allow finding input checking holes by inspecting the PHP
     code.  Hence the goal to locate, plug, and verify *all* such holes before
     open-source distribution.

     Some site mappers are combined with plugins to generate "blind" SQL
     injection probes against the input fields of forms.  They might be
     effective against a suicidal site with very little sanity checking on
     inputs.

     Our PHP code checks "almost all" inputs serially, so the first input that
     is rejected short-circuits checking the rest of the inputs and generating
     queries.  But a clever penetrator could generate reasonable inputs and
     get past that to find a hole and exploit it.  We need an automated way to
     provide some assurance that there are no holes to find.

   . I selected several of the tools to try:

     - Screaming Cobra - Mapper with form vulnerability "techniques".  (Perl)

       I tried it in the insulation of an Emulab-in-Emulab, with various
       combinations of arguments.  Not surprisingly, its blind thrashing
       didn't penetrate any of the public Emulab pages.

       But one could go much further with an Emulab login uid and password,
       after adding necessary login session cookie logic.  And further still
       if admin mode were breached...

     - Spike Proxy, an HTTP Hacking / Fuzzing proxy
       (Open Source, Python, Windows/Linux) w/ SQL injection.

       This is a good example of what a manual "penetration tester" (black or
       white hat) would use to attack a web site.

       I used it to record browsing sessions while manually operating the
       Emulab web site.  It also allows editing inputs and replaying attacks
       but I didn't try that.

       It was useful to determine exactly what POST arguments were passed and
       their values.  This broke a chicken-and-egg bootstrapping problem: I
       had to script "activation" of the DB (creating "one of everything")
       before I could spider the web site and enumerate the full set of forms
       arguments.

     - WebInject - Free automated test tool for web apps and services.
       Perl/XML, GPL'ed.

       This sounded perfect at first, particularly since it presents a GUI of
       the test results.  But to do that, it has to be provided with a
       complete set of success and failure match strings, expressed along with
       the URL's to probe in an XML file.

       Unfortunately, it's made for monolithic runs, where everything is in a
       single session.  That's not useful for incremental development,
       particularly since the the web pages are not retained to help
       understand what happened.

       Maybe this will be useful at some point though, so I kept it under
       sec-check with some stubs in the makefile for generating the XML.

       Meanwhile, I found that the (new version of the) venerable "wget"
       command has the necessary options to retain login cookies, set
       traversal limits for recursive spidering, and convert pages so they are
       browsable from local directories.  Far more convenient.

       I wound up implementing logic similar to WebInject in a much simpler
       way (but without a GUI.)
