/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2002-2006 University of Utah and the Flux Group.
 * All rights reserved.
 */
import java.awt.*;
import java.lang.*;

public class IFaceThingee extends Thingee {
    private Thingee a,b;

    private static Color ickyBrown;
    
    static {
	ickyBrown = new Color( 0.8f, 0.8f, 0.7f );
    }

    private void domove() 
    {
	// snap to unit circle from a in dir of b.
	float xd = b.getX() - a.getX();
	float yd = b.getY() - a.getY();
	float magSquared = (xd * xd + yd * yd);
	float mag = 14.0f / (float)Math.sqrt( magSquared );
	
	super.move( (int)((float)a.getX() + (xd * mag)),
		    (int)((float)a.getY() + (yd * mag)));
	/*
	super.move((a.getX() * 3 + b.getX()) / 4,
	           (a.getY() * 3 + b.getY()) / 4 );
	*/
    }

    public IFaceThingee( String newName, Thingee na, Thingee nb ) {
	super( newName );
	linkable = false;
	moveable = false;
	trashable = false;
	a = na;
	b = nb;
	domove();
    }

    public Thingee getA() { return a; }
    public Thingee getB() { return b; }

    public void move( int nx, int ny ) {
	// nope. can't allow this.
    }

    public int size() { return 9; }
    
    public boolean clicked( int cx, int cy ) {
	int x = getX();
	int y = getY();
	return ((cx - x) * (cx - x) + (cy - y) * (cy - y) < (9 * 9));
    }

    public Rectangle getRectangle() {
	return new Rectangle( getX() - 12, getY() - 12, 26, 26 );
    }

    public void drawIcon( Graphics g ) {
	//g.setColor( Color.lightGray );
	//g.fillOval( -6, -6, 16, 16 );

	g.setColor( ickyBrown );
	g.fillOval( -8, -8, 16, 16 );
 
	g.setColor( Color.black );
	g.drawOval( -8, -8, 16, 16 );	
    }

    public void draw( Graphics g ) {
	domove();
	g.setColor( Color.darkGray );
	//g.drawLine( a.getX(), a.getY(), b.getX(), b.getY() );
	super.draw( g );
    }

    public boolean isConnectedTo( Thingee t ) {
	return (a == t | b == t);
    }
}
