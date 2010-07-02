#
# EMULAB-COPYRIGHT
# Copyright (c) 2007 University of Utah and the Flux Group.
# All rights reserved.
#
sec-check/README-concepts.txt - Design and methods employed in sec-check.

For more details of running and incremental development, see README-howto.txt .
See README-FIRST.txt for a top-level outline.


 - Overview of the sec-check tool

   . This is an SQL injection vulnerability scanner, built on top of an
     automated test framework.  It could be factored into generic and
     Emulab-specific portions without much difficulty.

   . Drives the HTML server in an inner Emulab-in-Emulab experiment via wget,
     using forms page URL's with input field values.  Most forms-input values
     are automatically mined from HTML results of spidering the web interface.

   . This is "web scraping", not "screen scraping"
     http://en.wikipedia.org/wiki/Web_scraping

         Web scraping differs from screen scraping in the sense that a website
         is really not a visual screen, but a live HTML/JavaScript-based
         content, with a graphics interface in front of it. Therefore, web
         scraping does not involve working at the visual interface as screen
         scraping, but rather working on the underlying object structure
         (Document Object Model) of the HTML and JavaScript.

   . Implemented as an Emulab GNUmakefile.in for flexible control flow.

     - Some actions use gawk scripts to filter the results at each stage,
       generating inputs and/or scripts for the next state.


 - Several stages of operation are supported, each with analysis and summary,
   corresponding to the top-level sections of the GNUmakefile.in .

   For more details of running and incremental development, see README-howto.txt .
     "gmake all" to do everything after activation.
     "gmake msgs" to see all of the summaries.

   ----------------
   . src_forms:

     Grep the sources for <form and make up a list of php form files.

     Here's an example of the src_msg output:

     ** Sources: 107 separate forms are on 89 code pages. **
     ** (See src_forms.list and src_files.list
     **  in ../../../testbed/www/sec-check/results .) **

   ----------------
   . activate: 

     Sets up the newly swapped-in ElabInElab site in the makefile to create
     "one of everything" (or sometimes two in different states), thus turning
     on as many forms as we can for spidering.

     ** Activation analysis: success 12, failure 0, problem 0, UNKNOWN 0 **
     ** (See analyze_activate.txt in ../../../../testbed/www/sec-check/results .) **

     . Cookie logic for logout/login/admin actions is also in the makefile.

   ----------------
   . spider:

     Recursively wget a copy of the ElabInElab site and extract a <forms list.

     ** Spider: 1773 ( 3 + 1770 ) forms instances are in 55 ( 3 + 55 ) web pages. **
     ** (See *_{forms,files}.list in ../../../testbed/www/sec-check/results .) **

     - Actually, spider it twice, once not logged in for the public view,
       and again, logged in and with administrative privileges, for the
       private view.

     - Don't follow page links that change the login/admin state here.

     - Also reject other links to pages which don't have any input fields,
       and don't ask for confirmation before taking actions.  These must be
       tested specially.

   ----------------
   . forms_coverage:

     Compare the two lists to find uncovered (unlinked) forms.

     ** Forms: 34 out of 89 forms files are not covered. **
     ** (See ../../../testbed/www/sec-check/results/files_missing.list .) **

     - Generally, unlinked forms are a symptom of an object type (or state)
       that is not yet activated.  Iterate on the activation logic.

   ----------------
   . input_coverage:

     Extract <input fields from spidered forms.

     ** Inputs: 9965 input fields, 343 unique, 123 over-ridden. **
     ** (See site_inputs.list and input_names.list
     **  in ../../../testbed/www/sec-check/results,
     ** and input_values.list in ../../../testbed/www/sec-check .) **

     - form-input.gawk is applied to the spidered public and admin .html
       files to extract forms/input lists.

     - That process is generic, but there are a few little Emulab special
       cases in the makefile where they are combined into a single list.
       Special cases (hacks) are marked with XXX to make them easy to find.

     - Start making an input values over-ride dictionary to point the pages
       at the activation objects, using common input field names.

   ----------------
   . normal:

     Create, run, and categorize "normal operations" test cases.

     ** Run analysis: success 47, failure 6, problem 2, UNKNOWN 0 **
     ** (See analyze_output.txt in ../../../testbed/www/sec-check/results .) **

     - The forms/inputs list is combined with the input value over-ride
       dictionary using forms-to-urls.gawk, producing a list of forms page
       URL's with GET and/or POST arguments.

     - The url list is separated into setup, teardown, and show (other)
       sections using the sep-urls.gawk script.

       The {setup,teardown}_forms.list control files specify sequences of
       PHP pages in the order that their operations must be performed,
       e.g. creating a new project before making new experiments in the
       project.

     - A subtlety is that the activation objects are used by the "show"
       script, where the setup and teardown scripts leave those alone and
       suffix the ephemeral Emulab objects they create and delete with a
       "3".  There are many XXX special cases in sep-urls.gawk .

     - The separated url lists are transformed into scripts containing wget
       commands (generated by the urls-to-wget.gawk script) and run.

     - Iterate until everything works, categorizing UNKNOWN results with
       {success,failure,problem}.txt pattern lines until everything is
       known.  "Problems" are a small subset of failures, showing errors in
       the sequencing of operations, or broken page logic due to the testing
       environment, rather than input errors detected by the page logic.

     - Additional commands, prefixed with a "!" character, are added to the
       {setup,teardown}_forms.list files, which start to look more like
       scripts.

       Arbitrary commands can be ssh'ed to $MYBOSS and $MYOPS.

       There's a special "sql" pseudo-command that can be used for select
       queries to fetch values from the Emulab DB into shell variables, or
       update queries to set DB state.  (See urls-to-wget.gawk for details.)

       It's also useful to surround sections with conditional logic to check
       that necessary objects are in place to avoid a lot of unnecessary
       collateral damage from page failures.

   ----------------
   . probe:

     Create and run probes to test the checking code of all input fields.

     ** Probe analysis: 408 probes out of 408 executed: 166 showed success,
     **     242 failure (or probes caught), 38 dups, 0 UNKNOWN.
     ** Probes to 53 pages gave 13 hits: 13 backslashed, 0 UNCAUGHT in 0 pages.
     ** (See probe-labels.list and uncaught-files.list
     **  in ../../../testbed/www/sec-check/results .) **

     - SQL injection probes are special strings substituted for individual
       GET and POST arguments by the forms-to-urls.gawk script.  They start
       with an unmatched single-quote and are labeled with their form page
       and input field name, for example:
	   query_type='**{kb-search.php3:query_type}**

     - One page will be probed in the generated wget scripts as many times
       as it has input fields (up to 30 in one case.)  After the probes to a
       page, one "normal" wget line is generated to perform the page
       function and create the necessary conditions for going on to probe
       the next page.

     - A "probe catcher" is put into the underlying PHP common query
       function to look in constructed SQL query strings for the probe
       string prefix and throw an error if it's seen, with or without a
       backslash escaping the single-quote.  (This "hit" error message is
       included in the failure.txt file, so probe hits are also failures.)

       Obviously, the goal is to probe everything, and let no probe go
       uncaught or unescaped.

     - Sometimes, it's necessary to wait for a "backgrounded" page action to
       complete before going on in the script.  There's a "waitexp" helper
       script for the common case of waiting for an Emulab experiment to be
       in a particular state, "active" by default.

     - Many probe strings will be ignored or escaped by the page logic,
       causing the page to perform its function (such as creating or
       deleting a user, project, or experiment.)  There may some strange
       text included (or not, if only the presence of the argument is
       considered by the page logic.)

     - The failure.txt file is used to determine whether the page performed
       its function and if so to undo it.  "Undo" command lines are added
       after PHP page files in the {setup,teardown}_forms.list files,
       prefixed with a "-" character.  There's an undo_probes.pl script with
       common logic for a variety of Emulab object types.


    - Plug all of the holes by adding or fixing input validation logic.
      . Re-run probes to check.
      . Re-do it periodically, as the system evolves.

