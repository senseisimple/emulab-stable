/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */
/*
 * Some utility stuff.
 */

/*
 * Draw a box around template that is being displayed (in the graph).
 * This turns on a div after setting its x,y,w,h, which we glean from
 * from the coord string of the "area" element. Could be much smarter,
 * but fisrt I would have to be smarter.
 */
function SetActiveTemplate(img_name, div_name, area_name)
{
    var template_image = document.getElementById(img_name);
    var template_div   = document.getElementById(div_name);
    var template_area  = document.getElementById(area_name);

    /*
     * The area coords tell us where to place the red overlay box on the image.
     */
    var myarray = template_area.coords.split(",");

    var x1 = myarray[0] - 0;
    var y1 = myarray[1] - 0;
    var x2 = myarray[2] - 0;
    var y2 = myarray[3] - 0;

    var top    = template_image.offsetTop + y1;
    var left   = template_image.offsetLeft + x1;
    var width  = (x2 - x1) - 2;
    var height = (y2 - y1) - 2;

    template_div.style['top']    = top + "px";
    template_div.style['left']   = left + "px";
    template_div.style['height'] = height + "px";
    template_div.style['width']  = width + "px";
    template_div.style['visibility'] = "visible";
}

/*
 * For initializing template run params
 */
function SetRunParams(names, values)
{
    for (i = 0; i < names.length; i++) {
	var name  = names[i];
	var value = values[i];
	var field = document.getElementById("parameter_" + name);

	if (field) {
	    field.value = value;
	}
    }
}

