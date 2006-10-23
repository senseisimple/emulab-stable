#! /usr/bin/gawk -f
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
# All rights reserved.
#
# sep-urls - Separate out the setup and teardown URL's.
# 
#   Input is a list of .php3 filenames, then the URL's to be separated.
#   Output is the subset of URL's corresponding to the file list, in order.

# No colon, just a bare filename, so not a URL.
# Remember its name and output order.
!/:/ { order[$1] = ++nfiles; next; }

# Stash the desired URL's, indexed by their output order.
{
    # Remove suffix after php filename first, then path prefix.
    fn = gensub(".*/", "", 1, gensub("(php3*).*", "\\1", 1, $0));
    if ( o = order[fn] ) urls[o] = $0;
    ##printf "file %s, order %s, url %s\n", fn, o, $0;
}

# Dump the ones seen, in order.
END{
    for ( i = 1; i <= nfiles; i++ )
	if ( urls[i] != "" )
	    print urls[i];
}
