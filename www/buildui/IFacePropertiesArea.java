public class IFacePropertiesArea extends PropertiesArea
{
    public boolean iCare( Thingee t ) {
	return (t instanceof IFaceThingee);
    }

    public String getName() { return "Interface Properties"; }

    public IFacePropertiesArea() 
    {
	super();
	addProperty("ip", "IP address:","<auto>");
    }
};

/* lanlink
   delay/b/loss */
