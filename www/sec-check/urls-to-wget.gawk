#! /usr/bin/gawk -f
#
# forms-to-urls - Generate wget statements for a set of URL's.
#
# Assumes you are already logged in to Emulab, with a valid cookies.txt file.
#
#   Intput is a set of page URL's including appended ?args.
#   The Get arg method is default.  Post is indicated by a post: prefix.
#

BEGIN{
    verbose = "-S ";

    COOKIES	= "cookies.txt";
    ld_cookies	= "--load-cookies " COOKIES;
    # Don't get prerequisites (-p) so we can redirect the output page (-O).
    wget_args	= verbose "-k --keep-session-cookies --no-check-certificate"
}

{ 
    # Encode a few characters as %escapes.
    gsub(" ", "%20");
    gsub("!", "%21");
    gsub("\"", "%22");

    # Look for a "post:" prefix specifying the Post arg method.
    url = $0;
    post = match(url, "post:") != 0;
    if ( post ) url = substr(url, RLENGTH+1);

    # Parse the URL string.  "?args" would be optional, except that
    # there can be slashes in them, causing the host part to stretch.
    match(url, "^(http.*)/([^?]*)[?](.*)", p);
    ##printf "URL: [1] %s, [2] %s, [3] %s\n", p[1], p[2], p[3];

    if ( post )
	url_args = sprintf( "--post-data \"%s\" \"%s/%s\"", p[3], p[1], p[2]);
    else
	url_args = sprintf("\"%s\"", url);

    # Make a local destination file with a numeric suffix if needed.
    file = p[2];		# Null string initially, then 1, 2, 3...
    suffix = files[file];
    files[file]++;		# Increment for next time.
    if ( suffix ) file = file "." suffix;
    file = file ".html";	# html suffix for web browser.
    file_args = "-O " file;


    print "wget", wget_args, ld_cookies, file_args, url_args;
}
