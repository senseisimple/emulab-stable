/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2007 University of Utah and the Flux Group.
 * All rights reserved.
 */

/* 
 * Just a simple interface for dealing with select objects.
 */

dynselect = new Object();

/*
 * Return the first selected option.
 */
dynselect.getSelectedOption = function(sobj) {
    if (!objdef(sobj) || !objdef(sobj.options) || sobj.options.length < 1) {
	return null;
    }

    for (var i = 0; i < sobj.options.length; ++i) {
	if (sobj.options[i].selected) {
	    return sobj.options[i].text;
	}
    }

    return null;
}

/*
 * Remove a select object's current options and add ones with names 
 * (value=name) in the optNames array.  If selectedOpt is not null,
 * we set the option of that name to be selected.  If optValues is not null,
 * we look for optValues[optNames[i]] and set if possible; else, 
 */
dynselect.setOptions = function(sobj,optNames,selectedOpt,optValues) {
    if (!objdef(sobj) || !objdef(optNames) || optNames.length < 1) {
	return false;
    }

    // remove them all safely first
    for (var i = sobj.options.length - 1; i >= 0; --i) {
	sobj.remove(i);
    }

    for (var i = 0; i < optNames.length; ++i) {
	var isSelected = (optNames[i] == selectedOpt)?true:false;
	var optval = optNames[i];
	if (optValues != null && objdef(optValues[optNames[i]])) {
	    optval = optValues[optNames[i]];
	}

	// Doesn't seem to work in IE
	//sobj.add(new Option(''+optNames[i],''+optval,isSelected,isSelected),
	// null);
	sobj.options[sobj.options.length] = new Option(''+optNames[i],
						       ''+optval,
						       isSelected,isSelected);
    }

    return true;
}

dynselect.moveSelectedOptions = function(s1,s2) {
    if (!objdef(s1) || !objdef(s1.options) || s1.options.length < 1) {
	return false;
    }

    // gather up all the objects in s1 that are selected, then move to s2:
    var ta = new Array();
    for (var i = 0; i < s1.options.length; ++i) {
	if (s1.options[i].selected) {
	    ta.push(s1.options[i]);
	    s1.remove(i);
	    // for each item we remove, recheck this idx of course
	    --i;
	}
    }
    for (var i = 0; i < ta.length; ++i) {
	s2.options[s2.options.length] = ta[i];
    }
}

/*
 * Move selected options in a select up (dir=0) or down (dir=1).
 */
dynselect.moveSelectedOptionsVertically = function(sobj,dir) {
    if (!sobj.options || sobj.options.length < 1) {
	return false;
    }

    if (dir == 0) {
	// bubble them up, lowest first
	for (var i = 0; i < sobj.options.length; ++i) {
	    if (sobj.options[i].selected) {
		// swap with previous one (unless it's selected)
		if ((i - 1) >= 0 && !sobj.options[i - 1].selected) {
		    dynselect.swapOptions(sobj,i,i - 1);
		}
	    }
	}
    }
    else {
	for (var i = sobj.options.length - 1; i >= 0; --i) {
            if (sobj.options[i].selected) {
                if ((i + 1) < sobj.options.length 
                    && !sobj.options[i + 1].selected) {
                    dynselect.swapOptions(sobj,i,i + 1);
                }
            }
        }
    }

    return true;
}

dynselect.copySelectedOptions = function(s1,s2) {
    if (!s1.options || s1.options.length < 1) {
        return false;
    }

    for (var i = 0; i < s1.options.length; ++i) {
	if (s1.options[i].selected) {
	    dynselect.copyOption(s1,s2,i);
	}
    }

    return true;
}

dynselect.copyOption = function(s1,s2,i) {
    if (!s1.options || s1.options.length < 1) {
        return false;
    }
    if (i < 0 || i >= s1.options.length
        || j < 0 || j >= s1.options.length) {
	return false;
    }

    ec = new Option(s1.options[i].text,s1.options[i].value,
		    s1.options[i].defaultSelected,s1.options[i].selected);
    s2.options[s2.options.length] = ec;

    return true;
}

dynselect.swapOptions = function(sobj,i,j) {
    if (!sobj.options || sobj.options.length < 1) {
	return false;
    }
    if (i < 0 || i >= sobj.options.length 
	|| j < 0 || j >= sobj.options.length) {
        return false;
    }
    
    // we have to use temp objs heavily cause sometimes the browsers do
    // screwy stuff with select objects if we do a straight swap.
    e1 = new Option(sobj.options[i].text,sobj.options[i].value,
		    sobj.options[i].defaultSelected,sobj.options[i].selected);
    e2 = new Option(sobj.options[j].text,sobj.options[j].value,
		    sobj.options[j].defaultSelected,sobj.options[j].selected);

    // Hacks to fix background color of <option> elms when they get swapped.
    // Other style properties require similar hacks, I bet.
    e1.style.backgroundColor = sobj.options[i].style.backgroundColor;
    e2.style.backgroundColor = sobj.options[j].style.backgroundColor;

    e1s = sobj.options[i].selected;
    e2s = sobj.options[j].selected;
    sobj.options[j] = e1;
    sobj.options[j].selected = e1s;
    sobj.options[i] = e2;
    sobj.options[i].selected = e2s;
    
    return true;
}

dynselect.setAllSelectedVal = function(sobj,val) {
    if (!sobj.options || sobj.options.length < 1) {
	return false;
    }

    for (var i = 0; i < sobj.options.length; ++i) {
	sobj.options[i].selected = val;
    }

    return true;
}

dynselect.selectAll = function(sobj) {
    return dynselect.setAllSelectedVal(sobj,true);
}

dynselect.deselectAll = function(sobj) {
    return dynselect.setAllSelectedVal(sobj,false);
}

dynselect.getOptsAsDelimString = function(sobj,delim,onlySelectedOpts) {
    if (!objdef(sobj) || !objdef(sobj.options) || sobj.options.length < 1) {
	return '';
    }

    var ta = new Array();
    for (var i = 0; i < sobj.options.length; ++i) {
	if (!onlySelectedOpts 
	    || (onlySelectedOpts && sobj.options[i].selected)) {
	    ta.push(sobj.options[i].text);
	}
    }
    
    return (ta.join(delim));
}

function objdef(obj) {
    if (typeof(obj) != "undefined") {
	return true;
    }
    return false;
}
