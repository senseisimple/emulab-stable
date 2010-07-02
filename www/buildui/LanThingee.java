/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2002-2006 University of Utah and the Flux Group.
 * All rights reserved.
 */
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
