
public class NodePropertiesArea extends PropertiesArea
{
    public boolean iCare( Thingee t ) {
	return (t instanceof NodeThingee);
    }

    public String getName() { return "Node Properties"; }

    public NodePropertiesArea() 
    {
	super();
	addProperty("name", "name:","");
	addProperty("hardware", "hardware:","<auto>");
	addProperty("osid", "os id:","<auto>");
    }
};
