
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
	addProperty("hardware", "hardware:","<auto>", true, false );
	addProperty("osid", "os id:","<auto>", true, true );
    }
};
