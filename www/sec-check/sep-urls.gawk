#! /usr/bin/gawk -f
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
# All rights reserved.
#
# sep-urls - Separate out the setup and teardown URL's.
# 
#   Input is in two parts, concatenated: 
#     . An ordered list of .php3 filenames, with interspersed action lines.
#       - PHP filename lines have a / at the front to anchor them for
#         uniqueness.
#
#       - Comment lines are prefixed by a "#".  The list is also used as a
#         grep pattern file, so NO BLANK LINES!
#
#       - Action lines are prefixed by a "!" or "-".  See urls-to-wget.gawk
#         for further descriptions
#
#     . Then the URL's to be separated out and ordered according to the list.
#
#   Output is the subset of URL's corresponding to the file list, in order,
#   with action lines added.  Meant to be fed to urls-to-wget.gawk .
#
#   A -v SYSADMIN= awk arg specifies the administrator login name to use.
#
 
# Just a slash and filename, so not a URL.
# Remember its name and output order.
/^[/]/ { names[++nfiles] = name = substr($0,2); order[name] = nfiles; next; }

# Action lines start with an exclamation point or dash.  Just pass through.
/^!/ { urls[++nfiles ",1"] = $0; next; }
/^-/ { inverse[name] = $0; next; }

# Comments.
/^#/ { next; }

# Common code to register a URL for a file into the urls array.
function fn_url(fn, url) {
    # Maybe many URL's per filename when probing, so append a count subscript.
    if ( o = order[fn] ) urls[o "," (nn = ++n_urls[o])] = url;
    ##printf "file %s, order %s,%s , url %s\n", fn, o, nn, url;
}

# Stash the desired URL's, indexed by their output order.
{
    # XXX Bash the proj, group, user, exp ids to leave activation objs alone.
    # The setup and teardown sequences use a "3" suffix for their objects.
    $0 = gensub("(test(proj|group|exp|img|osid|imgid))[12]?", "\\13", "g", $0);
    $0 = gensub("(testuse?r)[12]?", "testusr3", "g", $0);
    $0 = gensub("(pid\\]?=)testbed", "\\1testproj3", "g", $0);

    # Extract the filename: Remove args suffix after php tail, then path prefix.
    fn = gensub(".*/", "", 1, gensub("(php3*).*", "\\1", 1, $0));

    # XXX A bunch of per-page special cases.  (Grumble.)
    #
    # Probe the login page separately; it messes up admin mode from the makefile now.
    if ( fn == "login.php3" ) {
	next;
    }
    # Now user e-mail addresses must be unique, as well as login names.
    else if ( fn == "joinproject.php3") {
	sub( "\\[usr_email\\]=" SYSADMIN "@", "[usr_email]=testusr3@");
    }
    # Approval field name for approveuser contains user name, postpone is default.
    # Can't substitute in input_values.list because the field name isn't known.
    else if ( fn == "approveuser.php3") {
	sub("=postpone", "=approve");
    }
    # REALLY REALLY confirm freezeuser, deleteuser, and deleteproject.
    if ( fn == "freezeuser.php3" || fn == "deleteuser.php3" \
	 || fn == "deleteproject.php3") {
	sub("confirmed=Confirm", "confirmed=Confirm\\&confirmed_twice=Confirm");

	# Have to do deleteuser twice.
	if ( fn == "deleteuser.php3") {  
	    # The first time removes the user from the project.
	    fn_url(fn, $0);

	    # The second time removes the project-less user from the system.
	    sub("&target_pid=testproj3", "");
	}
    }

    fn_url(fn, $0);
}

# Dump the ones seen, in order, with action lines interspersed.
# Complain about the ones not seen.
END{
    for ( i = 1; i <= nfiles; i++ )
	if ( length(urls[i ",1"]) )
	    for ( n = 1; length( url = urls[i "," n] ); n++ ) {
		print url;
		# Append "inverse action" lines.
		inv = inverse[names[i]];
		if ( length(inv) ) print inv;
	    }
	else if ( names[i] != "login.php3" ) # Login is handled specially.
		print "*** Missing:", i, names[i] > "/dev/stderr"
}
