/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2002-2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

public class NodePropertiesArea extends PropertiesArea
{
    public boolean iCare( Thingee t ) {
	return (t instanceof NodeThingee);
    }

    public String getName() { return "Node Properties"; }

    public NodePropertiesArea() 
    {
	super();
	addProperty("name", "name:","", true, false);
	addProperty("hardware", "hardware:","<auto>", true, true );
	addProperty("osid", "os id:","<auto>", true, true );
    }
};
