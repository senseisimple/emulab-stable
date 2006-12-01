/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2002-2006 University of Utah and the Flux Group.
 * All rights reserved.
 */
import java.awt.*;

class LanLinkThingee extends LinkThingee {

    private static Color paleBlue;
    
    static {
	paleBlue = new Color( 0.8f, 0.8f, 1.0f );
    }

    public void drawIcon( Graphics g ) {
	g.setColor( Color.lightGray );
	g.fillRect( -6, -6, 16, 16 );

	g.setColor( paleBlue );
	g.fillRect( -8, -8, 16, 16 );
 
	g.setColor( Color.black );
	g.drawRect( -8, -8, 16, 16 );	
    }

    public LanLinkThingee( String newName, Thingee na, Thingee nb ) {
	super( newName, na, nb );
    }
}
