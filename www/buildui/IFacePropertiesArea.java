/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2002-2006 University of Utah and the Flux Group.
 * All rights reserved.
 */
public class IFacePropertiesArea extends PropertiesArea
{
    public boolean iCare( Thingee t ) {
	return (t instanceof IFaceThingee);
    }

    public String getName() { return "Interface Properties"; }

    public IFacePropertiesArea() 
    {
	super();
	addProperty("ip", "IP address:","<auto>", false, false);
    }
};

/* lanlink
   delay/b/loss */
