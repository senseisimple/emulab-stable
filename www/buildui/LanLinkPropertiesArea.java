public class LanLinkPropertiesArea extends PropertiesArea
{
    public boolean iCare( Thingee t ) {
	return t instanceof LanLinkThingee;
    }

    public String getName() { return "LAN Link Properties"; }

    public LanLinkPropertiesArea() 
    {
	super();
	addProperty("bandwidth", "bandwidth(Mb/s):", "100", false, false);
	addProperty("latency", "latency(ms):", "0", false, false);
	addProperty("loss", "loss rate(0.0-1.0):", "0.0", false, false);
    }
};
