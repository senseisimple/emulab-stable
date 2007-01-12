#! /usr/bin/gawk -f
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
# All rights reserved.
#
# sep-urls - Separate out the setup and teardown URL's.
# 
#   Input is a list of .php3 filenames, then the URL's to be separated.
#   PHP filenames have a / at the front to anchor them for uniqueness.
#   Interspersed action lines may be prefixed by a "!".
#   Output is the subset of URL's corresponding to the file list, in order.
#
#   A -v SYSADMIN= awk arg specifies the administrator login name to use.
 
# Just a slash and filename, so not a URL.
# Remember its name and output order.
/^[/]/ { names[++nfiles] = name = substr($0,2); order[name] = nfiles; next; }

# Action lines start with an exclamation point.  Just pass through.
/^!/ { urls[++nfiles] = $0; next; }

# Special cases.  (Grumble.)
# Probe the login page separately; it messes up admin mode from the makefile now.
index($0, "/login.php3?") { next; }
# Now user e-mail addresses must be unique, as well as login names.
index($0, "/joinproject.php3?") {
    sub( "\\[usr_email\\]=" SYSADMIN "@", "[usr_email]=testusr@");
}

# Stash the desired URL's, indexed by their output order.
{
    # Change proj, group, user, exp ids to leave activation objs alone.
    $0 = gensub("(test(proj|group|exp))[12]?", "\\13", "g", $0);
    $0 = gensub("(testuse?r)[12]?", "testusr3", "g", $0);
    $0 = gensub("(pid\\]?=)testbed", "\\1testproj3", "g", $0);

    # Remove suffix after php filename first, then path prefix.
    fn = gensub(".*/", "", 1, gensub("(php3*).*", "\\1", 1, $0));
    if ( o = order[fn] ) urls[o] = $0;
    ##printf "file %s, order %s, url %s\n", fn, o, $0;
}

# Dump the ones seen, in order, with action lines interspersed.
# Complain about the ones not seen.
END{
    for ( i = 1; i <= nfiles; i++ )
	if ( urls[i] != "" ) print urls[i];
	else if ( names[i] != "login.php3" ) # Login is handled specially.
		print "*** Missing:", names[i] > "/dev/stderr"
}
