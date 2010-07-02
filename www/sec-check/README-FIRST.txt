#
# EMULAB-COPYRIGHT
# Copyright (c) 2007 University of Utah and the Flux Group.
# All rights reserved.
#
sec-check/README-FIRST.txt - Sec-check documentation outline.


 . Goals
   - Purpose: Locate and plug all SQL injection holes in the Emulab web pages.
              Guide plugging them and repeat to find any new ones we introduce.
   - Useful as a test harness, even if not probing.
   - Method: Combine white-box and black-box testing, with much automation.


 . Background (See sec-check/README-background.txt)
   - SQL Injection vulnerabilities: Ref "The OWASP Top Ten Project"...
   - Automated vulnerability scan tools, search and conclusions...


 . Sec-check concepts. (See sec-check/README-concepts.txt)

   - Overview of sec-check tool

     . This is an SQL injection vulnerability scanner, built on top of an
       automated test framework.  It could be factored into generic and
       Emulab-specific portions without much trouble.

     . Drives the HTML server in an inner Emulab-in-Emulab experiment via wget,
       using forms page URL's with input field values.  Most forms-input values
       are automatically mined from HTML results of spidering the web interface.

     . This is "web scraping", not "screen scraping"...
     . Implemented as an Emulab GNUmakefile.in for flexible control flow...

   - Several stages of operation are supported, each with analysis and
     summary...

     . src_forms:
           Grep the sources for <form and make up a list of php form files.
     . activate: 
           Sets up the newly swapped-in ElabInElab site in the makefile...
     . spider:
           Recursively wget a copy of the ElabInElab site and extract a <forms list.
     . forms_coverage:
           Compare the two lists to find uncovered (unlinked) forms.
     . input_coverage:
           Extract <input fields from spidered forms.
     . normal:
           Create, run, and categorize "normal operations" test cases.
     . probe:
           Create and run probes to test the checking code of all input fields.


 . Details of running and incremental development (See README-howto.txt)

   - General
     . Directories
     . Inner Emulab-in-Emulab experiment

   - High-level targets
     . all: src_forms spider forms_coverage input_coverage normal probe
     . msgs: src_msg site_msg forms_msg input_msg analyze probes_msg

   - Stages of operation (makefile targets)
     . src_forms: src_list src_msg
     . activate: activate.wget $(activate_tasks) analyze_activate
     . spider: clear_wget_dirs do_spider site_list site_msg
     . forms_coverage: files_missing forms_msg
     . input_coverage: input_list input_msg
     . normal: gen_all run_all analyze
     . probe: gen_probes probe_all probes_msg
