public class LanPropertiesArea extends PropertiesArea
{
    public boolean iCare( Thingee t ) {
	return (t instanceof LanThingee);
    }

    public String getName() { return "Lan Properties"; }

    public LanPropertiesArea() 
    {
	super();
	addProperty("name", "name:","");
	addProperty("bandwidth", "bandwidth(Mb/s):", "100");
	addProperty("latency", "latency(ms):", "0");
	addProperty("loss", "loss rate(0.0 - 1.0):", "0.0");
    }
};
