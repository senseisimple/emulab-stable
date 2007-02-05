#! /usr/bin/gawk -f
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
# All rights reserved.
#
# forms-to-urls - Generate URL's for accessing the site.
#
#   form-input.gawk's output format is the input format for this script.
#
#   A site_values.list file path is provided by a -v VALUES=... awk arg.
#
#   . Contents are 'name="..." value'.  An optional value (separated by a
#     space character, extending to the end of line) is the default used for
#     auto-form-fill-in.  Names may be of the form array[element] as in PHP.
#
#   . Specifying !name="..." (an exclamation-point prefix) causes matching
#     input fields to be skipped.  If the name ends with a "*" then the name
#     is a prefix and all names _starting with_ that string are skipped.  Note
#     that "[" characters in names are not treated specially, so e.g. you can
#     skip a whole array by specifying !name="...[*" .
#
#   . The value may be prefixed with a ! to cause it to over-ride an action=
#     argument in the form page URL.  It follows that a value of just an "!"
#     specifies a null string value.
#
#   . The value may contain a %d, which is replaced with a disambiguating number
#     for argument values after the first.  (The first one just gets a null
#     string, as in the output file names in urls-to-wget.gawk .)  This is
#     useful for probing, where multiple probes will be generated for a single
#     page but the values can conflict.
#
#   Output is a set of page URL's including appended ?args.
#   The GET arg method is default, including action= args for a POSTed form.
#   A POST argument string follows a "?post:" separator after the other ?args.
#
#   A -v MAX_TIMES= awk arg specifies how many times to target a form.
#
#   A -v PROBE=1 awk arg turns on SQL injection probing.  A separate URL is
#   generated for each ?argument, substituting a labeled mock SQL injection
#   attack probe string for the proper value.
#
BEGIN {
    if ( ! MAX_TIMES ) MAX_TIMES = 1; # Default.

    while ( getline <VALUES ) {
	arg_name = $1;
	arg_name = gensub("name=\"([^\"]*)\"", "\\1", 1, arg_name);
	if ( substr($1, 1, 1) == "!" ) {
	    arg_name = substr(arg_name, 2);
	    ##print "not", arg_name;
	    if ( substr(arg_name, length(arg_name)) == "*" )
		skip_prefix[substr(arg_name, 1, length(arg_name)-1)] = 1;
	    else skip_name[arg_name] = 1;
	}
	if (NF > 1)
	    defaults[arg_name] = substr($0, index($0, $2));
	##printf "defaults %s=%s.\n", arg_name, defaults[arg_name];
    }
}

/^[.][\/]/ {			# A page file section starts with a filename.
    # Remember the host path from filenames.  Often not on <form action= .
    host_path = gensub("^[.][/](.*)/.*", "\\1", 1);
    ##print "host_path", host_path;
}

/^<form/ {			# <form action="..." method="..."
    action = gensub(".* action=\"([^\"]*)\".*", "\\1", 1);
    method = gensub(".* method=\"([^\"]*)\".*", "\\1", 1);

    # Action= URL can have args specified.  Use the values over anything else,
    # unless the default value is prefixed with a ! .  Keep them separate from
    # POST args because PHP code may get them through the $_GET array.
    url = action;
    action_file = gensub(".*/", "", 1, gensub("?.*", "", 1, url));
    delete args; delete action_args;
    if ( q = index(action, "?") ) {
	url = substr(action, 1, q-1);

	# The "&" arg separator is escaped in HTML.
	n = split(substr(action, q+1), url_args, "&amp;");
	for (i = 1; i <= n; i++) {
	    name_val = url_args[i];
	    eq = index(name_val, "=");
	    nm = substr(name_val, 0, eq-1);
	    vl = substr(name_val, eq+1);

	    # Input fields to be skipped.
	    if ( skip_name[nm] ) continue;
	    for (j = 1; j <= length(nm); j++ )
		if ( skip_prefix[substr(nm, 1, j)] ) continue;

	    action_args[nm] = vl;

	    # A default with a ! prefix over-rides an action= arg.
	    df = defaults[nm];
	    if ( df ~ "!" )
		action_args[nm] = substr(df, 2);
	    ##printf "name_val %s, nm %s, vl %s, df %s\n", name_val, nm, vl, df;
	}
    }

    # Add host path to relative url's.
    if (! index(url, ":") ) url = "https://" host_path "/" url;
    
    ##printf "url %s, file %s, method %s, action args", url, action_file, method;
    ##for (i in action_args) printf " %s", action_args[i]; printf "\n";

    target[url]++;
    form = target[url] <= MAX_TIMES; # Limit target hits.
    arg_vals = 0;		# Count arguments with user provided values.
}

form && /^<input/ {		# <input type="..." name="..." value=... ...>
    # Gotta have a name to be an arg.
    if ( $0 !~ " name=" ) next;

    # Type and name have been double-quoted.  Value can be single- or double-.
    type = gensub(".* type=\"([^\"]*)\".*", "\\1", 1);
    name = gensub(".* name=\"([^\"]*)\".*", "\\1", 1);

    # Input fields to be skipped.
    if ( skip_name[name] ) next;
    for (j = 1; j <= length(name); j++ )
	if ( skip_prefix[substr(name, 1, j)] ) next;

    if ( $0 ~ " value=\"" )
	value = gensub(".* value=\"([^\"]*)\".*", "\\1", 1);
    else if ( $0 ~ " value='" )
	value = gensub(".* value='([^']*)'.*", "\\1", 1);
    else value = "";
    checked = $0 ~ "\\<checked\\>";
    ##printf "type %s, name %s, value %s, checked %s\n", type, name, value, checked;

    val_arg = (type=="text" || type=="textarea" || type=="password" || 
	       type=="hidden" || type=="checkbox" || type=="select" ||
	       type=="radio" && checked);
    # Follow just the positive submit controls, not cancel, etc.
    sub_arg = (type=="submit" && 
	       (value ~ "Submit" || value ~ "Create" || 
		value=="Confirm" || value=="Go!"));

    if ( val_arg || sub_arg ) {
	##printf "name %s, default=%s, value=%s.\n", 
	##       name, defaults[name], value;
	df = defaults[name];
	if ( df != "" ) {
	    # Default value from VALUES file.  May have ! prefix.
	    if ( df ~ "!" )
		args[name] = substr(df, 2);
	    else
		args[name] = df;
	}
	else if ( value != "" )
	    # Value from <input field default.
	    args[name] = value;
	else
	    args[name] = "";

	if ( args[name] ) arg_vals++;
    }
}

form && /^$/ {			# Blank line terminates each form section.

    # Collect the arg strings, with action args first.
    arg_str = ""; n_args1 = n_args2 = 0;
    for (arg in action_args) {
	sep = ( n_args1==0 ? "?" : "&" );
	arg_str = arg_str sep arg "=" action_args[arg];
	n_args1++;
    }
    for (arg in args) {  # Form input field args, may be POSTed.
	if ( n_args2 != 0 ) sep = "&";
	else if ( method == "post" ) sep = "?post:";
	else sep = ( n_args1 == 0 ? "?" : "&" );
	arg_str = arg_str sep arg "=" args[arg];
	n_args2++;
    }

    if (arg_vals) {		# Ignore if no argument values to supply.

	if ( ! PROBE ) {
	    # Not probing.
	    gsub("%d", "", arg_str);
	    print url arg_str;
	}
	else {
	    # Substitute a labeled mock SQL injection attack probe string for
	    # EACH ?argument value.  Generates N urls.
	    delete all_args;
	    for (arg in action_args) all_args[arg] = action_args[arg];
	    for (arg in args) all_args[arg] = args[arg];
	    for (arg in all_args) {
		lbl = "**{" action_file ":" arg "}**";

		# Disambiguating number for %d.  Null string for the first one.
		dn_str = gensub("%d", dnum++, "g", arg_str);

		# Quote regex metachars in array argument names for matching.
		a = gensub("\\[", "\\\\[", 1, gensub("\\]", "\\\\]", 1, arg));
		a = gensub("\\$", "\\\\$", "g", a);

		# Notice the single-quote at the head of the inserted probe string.
		probe_str = gensub("(\\<" a ")=([^?&]*)", "\\1='" lbl, 1, dn_str);

		print url probe_str;
	    }
	}
    }
}
