import java.awt.*;

public class LanThingee extends IconThingee {
    static Image icon;

    public String getIconName() { return "lanicon.gif"; }

    public void drawIcon( Graphics g ) {
	if (icon == null) { icon = loadIcon(); }
	super.drawIcon( g, icon );
    }

    public LanThingee( String newName ) {
	super( newName );
    }
};
