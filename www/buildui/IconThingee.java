import java.awt.*;
import java.awt.image.*;

abstract public class IconThingee extends Thingee
implements ImageObserver {
    abstract public String getIconName();

    protected Image loadIcon() {
	return Netbuild.getImage( getIconName() );
    }

    public void drawIcon( Graphics g, Image icon ) {
	g.setColor( Color.lightGray );
	g.fillRect( -12, -12, 32, 32 );
	g.setColor( Color.white );
	g.fillRect( -16, -16, 32, 32 );
	g.setColor( Color.black );
	g.drawRect( -16, -16, 32, 32 );

	try {
	    if (icon != null) { g.drawImage(icon, -16, -16, this); }
	} catch (Exception e) { 
	    System.out.println( e.getMessage() );
	    e.printStackTrace();
	}
    }

    public IconThingee( String newName ) {
	super( newName );
    }

    public boolean imageUpdate( Image img, 
				int infoflags,
				int x, int y, 
				int width, int height ) {
	if (infoflags == ALLBITS) { Netbuild.redrawAll(); return false;}
	return true;
    }
};
