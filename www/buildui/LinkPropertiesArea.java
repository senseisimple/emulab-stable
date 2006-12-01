/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2002-2006 University of Utah and the Flux Group.
 * All rights reserved.
 */
public class LinkPropertiesArea extends PropertiesArea
{
    public boolean iCare( Thingee t ) {
	return (t instanceof LinkThingee && !(t instanceof LanLinkThingee));
    }

    public String getName() { return "Link Properties"; }
 
    public LinkPropertiesArea() 
    {
	super();
	addProperty("name", "name:", "", true, false);
	addProperty("bandwidth", "bandwidth(Mb/s):", "100", false, false);
	addProperty("latency", "latency(ms):", "0", false, false);
	addProperty("loss", "loss rate(0.0 - 1.0):", "0.0", false, false);	
    }
};
