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
#   A site_values.list file path is provided by a -v VALUES= awk arg.
#   Contents are 'name="..." value'.  Optional value (to end of line)
#   is used for auto-form-fill-in.
#
#   Output is a set of page URL's including appended ?args.
#   The Get arg method is default.  Post is indicated by a post: prefix.
#
#   A -v MAX_TIMES= awk arg specifies how many times to target a form.
#

BEGIN {
    if ( ! MAX_TIMES ) MAX_TIMES = 1; # Default.

    while ( getline <VALUES ) {
	arg_name = $1;
	arg_name = gensub("name=\"([^\"]*)\"", "\\1", 1, arg_name);
	###arg_name = gensub("formfields\\[(.*)\\]", "\\1", 1, arg_name);
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

    # Action= URL can have args specified.  Use the values over anything else.
    url = action;
    delete args; 
    if ( q = index(action, "?") ) {
	url = substr(action, 1, q-1);
	# The "&" arg separator is escaped in HTML.
	n = split(substr(action, q+1), url_args, "&amp;");
	for (i = 1; i <= n; i++) {
	    name_val = url_args[i];
	    eq = index(name_val, "=");
	    nm = substr(name_val, 0, eq-1);
	    vl = substr(name_val, eq+1);
	    args[nm] = vl;

	    # A default with a ! prefix over-rides an action= arg.
	    df = defaults[nm];
	    if ( df ~ "!" )
		args[nm] = substr(df, 2);
	    ##printf "name_val %s, nm %s, vl %s, df %s\n", name_val, nm, vl, df;
	}
    }

    # Add host path to relative url's.
    if (! index(url, ":") ) url = "https://" host_path "/" url;
    
    ##printf "url %s, method %s, args", url, method;
    ##for (i in args) printf " %s", args[i]; printf "\n";

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
    if ( $0 ~ " value=\"" )
	value = gensub(".* value=\"([^\"]*)\".*", "\\1", 1);
    else if ( $0 ~ " value='" )
	value = gensub(".* value='([^']*)'.*", "\\1", 1);
    else value = "";
    ##printf "type %s, name %s, value %s\n", type, name, value;

    val_arg = (type=="text" || type=="textarea" || type=="password" || 
	       type=="hidden" || type=="checkbox" || type=="select");
    # Follow just the positive submit controls, not cancel, etc.
    sub_arg = (type=="submit" && 
	       (value ~ "Submit" || value ~ "Create" || 
		value=="Confirm" || value=="Go!"));

    if ( val_arg || sub_arg ) {
	arg_name = name; ### gensub("formfields\\[(.*)\\]", "\\1", 1, name);
	##printf "arg_name %s, default=%s, value=%s.\n", 
	##       arg_name, defaults[arg_name], value;
	df = defaults[arg_name];
	if ( df != "" ) {
	    # Default value from VALUES file.  May have ! prefix.
	    if ( df ~ "!" )
		args[arg_name] = substr(df, 2);
	    else
		args[arg_name] = df;
	}
	else if ( value != "" )
	    # Value from <input field default.
	    args[arg_name] = value;
	else
	    args[arg_name] = "";

	if ( args[arg_name] ) arg_vals++;
    }
}

form && /^$/ {			# Blank line terminates each form section.
    arg_str = "";
    for (arg in args) {
	###if ( args[arg] != "" )
	    if ( arg_str == "" ) arg_str = arg "=" args[arg];
	    else arg_str = arg_str "&" arg "=" args[arg];
    }

    post = (method=="post" ? "post:" : "");
    if (arg_vals)		# Ignore if no argument values to supply.
	print post url "?" arg_str;
}    
