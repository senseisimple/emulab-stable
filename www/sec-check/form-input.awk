#! /usr/bin/awk -f
FNR == 1 { 
    form=0;

    # Exempt forms in twik and flyspray files.
    exempt = FILENAME ~ "/(twiki|flyspray)/";
    if ( exempt ) next;

    if (NR != 1) printf "\n";
    print FILENAME;
}

/<form/ && ! exempt && !/action=[^ ]*\/search.php3/ { 
    form=1;

    sub(".*<form", "<form");	# Put <form at beginning of line.
    sub("[ \t]on[a-zA-Z]+=.*['\"]", "", $0 ); # Skip Javascript.

    while ( !match($0, ">") ) { # Multi-line <form statements.
	sub("[ \t]*$", " ");	# Single space at end of line.
	getline ln;
	sub("[ \t]on[a-zA-Z]+=.*['\"]", "", ln ); # Skip Javascript.

	sub("^[ \t]*", "", ln); # No space on start of new line.
	$0 = $0 ln;
    }

    sub(">.*", ">");		# Leave only <form ... > on the line.
    print;
}

form && /<input/ {
    sub(".*<input", "<input");	# Put <input at beginning of line.
    sub("[ \t]on[a-zA-Z]+=.*['\"]", "", $0 ); # Skip Javascript.

    while ( !match($0, ">") ) { # Multi-line <input statements.
	sub("[ \t]*$", " ");	# Single space at end of line.
	getline ln;
	sub("[ \t]on[a-zA-Z]+=.*['\"]", "", ln ); # Skip Javascript.

	sub("^[ \t]*", "", ln); # No space on start of new line.
	$0 = $0 ln;
    }
    sub(">.*", ">");		# Leave only <input ... > on the line.

    # Canonicalize.
    sub("type=readonly", "type=text"); # There is no readonly type, text is default.
    # Convert single-quoted type and name values to double quotes.
    $0 = gensub("(name|type)='([^']+)'", "\\1=\"\\2\"", "g");
    # Quote unquoted values.
    $0 = gensub("(name|type|value)=([^'\"][^ >]+)", "\\1=\"\\2\"", "g");
    # Reorder: <input type=.* name=.* value=.* .*>
    $0 = gensub("<input (.*)value=('[^']+'|\"[^\"]+\")", "<input value=\\2 \\1", 1);
    $0 = gensub("<input (.*)name=('[^']+'|\"[^\"]+\")", "<input name=\\2 \\1", 1);
    $0 = gensub("<input (.*)type=('[^']+'|\"[^\"]+\")", "<input type=\\2 \\1", 1);
    gsub("  *", " ");		# Collapse extra spaces.

    print;
}

/<\/form/ { form=0 }

{next}
