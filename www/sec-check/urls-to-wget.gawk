#! /usr/bin/gawk -f
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
# All rights reserved.
#
# urls-to-wget - Generate a wget script for a set of URL's.  The script
# assumes you are already logged in to Emulab, with a valid cookies.txt file.
#
#   Input is a set of page URL's including appended ?args, from sep-urls.gawk .
#   Interspersed action lines for setup and teardown may be prefixed by a
#   "!" or "-".  See below for further description of "-" lines.
#
#   The GET arg method is default, including action= args for a POSTed form.
#   A POST argument string follows a "?post:" separator after the other ?args.
#
#   A -v COOKIES= awk arg gives the path to an alternate cookies.txt file.
#   A -v OUTDIR= awk arg gives the path to an alternate output directory.
#   (Otherwise, .html output files go into the current directory.)
#   A -v SRCDIR= awk arg gives the path to the script source directory.
#   A -v FAILFILE=/failure.txt awk arg enables generation of conditional
#        probe inverse "-" action lines.  See below.
#
#   Interspersed action lines may be prefixed by a "!" or a "-".  "!" lines
#   are just put into the output script among the wget lines.
#
#   When probing, an action line prefixed by a "-" that follows a /filename
#   line in the file list gives a corresponding "inverse action" to undo it if
#   the probe execution *DOESN'T FAIL*, due to ignoring the probe value given
#   for the input field.  E.g, the first beginexp that succeeds, uses up the
#   experiment name and blocks all other probes, so the experiment has to be
#   deleted again before the next probe is done.
#
#   "-" lines are wrapped in an "if" test so they only run if the preceding
#   probe wget output file DOES NOT match any grep pattern in failure.txt .
#   NOTE: This will only work reliably with a completed failure.txt pattern file,
#   i.e. no remaining "UNKNOWN" entries in the analyze_output.txt file.
#
#   -wget lines will have the rest of the URL and option arguments filled in.
#   !sql or -sql lines are quoted queries that are passed to the inner boss tbdb.
#   !varnm=sql is a variant for getting stuff from the DB into a shell variable.
#   Other "-" or "!" action lines are put into the shell script verbatim.

BEGIN{
    verbose = "-S ";

    if ( COOKIES == "" ) COOKIES = "cookies.txt";
    ld_cookies	= "--load-cookies " COOKIES;

    outpath = OUTDIR;
    if ( length(outpath) && !match(outpath, "/$") ) outpath = outpath "/";

    # Don't get prerequisites (-p) so we can redirect the output page (-O).
    wget_args	= verbose "-k --keep-session-cookies --no-check-certificate"
}

# Action lines.
/^!/ || /^-/ {
    type = substr($0, 1, 1);
    if ( $0 ~ /^.wget / ) {
	process_url(last_prefix "/" $2); # Sets url, url_args, post_args.
        # Put the undo output in a separate subdir to avoid confusion.
	file_args = "-O " outpath "undo/" last_file;
	cmd = sprintf("wget %s %s %s%s %s", 
		      wget_args, ld_cookies, file_args, post_args, url_args);
    }
    else if ( $0 ~ /^.sql / ) \
	cmd = "echo " substr($0, 5) "| ssh $MYBOSS mysql tbdb";
    else if ( match($0, /^.(\w+)=sql (.*)/, s ) ) \
	cmd = "set " s[1] "=`echo " s[2] "| ssh $MYBOSS mysql tbdb | tail +2`" \
	      "; echo \"    " s[1] " = $" s[1] "\""; # Show the value too.
    else cmd = substr($0, 2);

    # Unconditional action lines start with an exclamation point.
    if ( type == "!" ) { print "    " cmd; }
    # Conditional "inverse action" lines start with a dash.
    else if ( length(FAILFILE) ){ # Only need to undo when probing.
	# Wrap in an if-block.
	printf "if ( ! { grep -q -f %s/%s %s%s } ) then\n    %s\nendif\n",
	    SRCDIR, FAILFILE, outpath, last_file, cmd;
    }
    next;
}	

function process_url(u) {	# Sets url, url_args, post_args.
    # Encode a few characters as %escapes.
    gsub(" ", "%20");
    gsub("!", "%21");
    gsub("\"", "%22");
    gsub("#", "%23");
    gsub("\\$", "%24");
    ###gsub("/", "%2F");

    # Separate off a "?post:" argument string at the end of the URL.
    url = u;
    if ( post = match(url, "?post:") != 0 ) {
	post_args = sprintf(" --post-data \"%s\"", substr(url, RSTART+6));
	url = substr(url, 1, RSTART-1);
	##printf "URL %s, POST_ARGS %s\n", url, post_args;
    }
    else post_args = "";
    url_args = sprintf("\"%s\"", url);
}

# URL lines.
{
    process_url($0);

    # Parse the URL string.  "?args" would be simple, except that
    # there can be slashes in them, causing the host part to stretch.
    match(url, "^(http.*)/([^?]*)[?]?(.*)", p);
    ##printf "URL: [1] %s, [2] %s, [3] %s\n", p[1], p[2], p[3];

    # Make a local destination file, with a numeric suffix if needed.
    # (We may hit the same page many times when probing.)
    prefix = last_prefix = p[1];
    file = p[2];
    suffix = files[file];	# Null string initially, then 1, 2, 3...
    files[file]++;		# Increment for next time.
    if ( suffix ) file = file "." suffix;
    file = last_file = file ".html";	# html suffix for web browser.
    file_args = "-O " outpath file;

    print "wget", wget_args, ld_cookies, file_args post_args, url_args;
}
