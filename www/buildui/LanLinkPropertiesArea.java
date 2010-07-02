/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2002-2006 University of Utah and the Flux Group.
 * All rights reserved.
 */
public class LanLinkPropertiesArea extends PropertiesArea
{
    public boolean iCare( Thingee t ) {
	return t instanceof LanLinkThingee;
    }

    public String getName() { return "LAN Link Properties"; }

    public LanLinkPropertiesArea() 
    {
	super();
	addProperty("bandwidth", "bandwidth(Mb/s):", "<auto>", false, false);
	addProperty("latency", "latency(ms):", "<auto>", false, false);
	addProperty("loss", "loss rate(0.0-1.0):", "<auto>", false, false);
    }
};
