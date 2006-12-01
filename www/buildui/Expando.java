/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2002-2006 University of Utah and the Flux Group.
 * All rights reserved.
 */
import java.awt.*;
import java.awt.event.*;

class Expando extends Component
{
    private String text;
    private boolean expanded;
    private ActionListener actionListener;
    private static Color darkBlue;

    static {
	darkBlue = new Color( 0.3f, 0.3f, 0.5f );
    }

    public void setState( boolean state ) { expanded = state; }
    public boolean getState() { return expanded; }

    public Expando( String t)
    {
	super();
	text = t;
	expanded = true;

	setCursor( Cursor.getPredefinedCursor( Cursor.HAND_CURSOR ) );

	enableEvents( AWTEvent.MOUSE_EVENT_MASK );
	enableEvents( AWTEvent.MOUSE_MOTION_EVENT_MASK );
    }

    protected void processMouseEvent( MouseEvent e )
    {
	if (actionListener != null) {
	    if (e.getID() == MouseEvent.MOUSE_PRESSED) {
		expanded = !expanded;
		ActionEvent ae = new ActionEvent( this, ActionEvent.ACTION_PERFORMED,
						  expanded ? "down" : "up", 0 );
		actionListener.actionPerformed( ae );
		repaint();
	    }
	}
    }

    public void addActionListener( ActionListener l ) 
    {
	actionListener = l;
    }

    public void removeActionListener( ActionListener l )
    {
	actionListener = null;
    }

    public Dimension getPreferredSize()
    {
	Graphics g = getGraphics();
	if (g != null) {
	    FontMetrics fm = g.getFontMetrics();
	    if (fm != null) {
		int stringWidth = fm.stringWidth(text);
		return new Dimension(16 + 4 + stringWidth, 16);
	    }
	    g.dispose();
	} 
	
	return new Dimension(32, 32);
    }

    public Dimension getMinimumSize() { return getPreferredSize(); }
    public Dimension getMaximumSize() { return getPreferredSize(); }

    public void paint( Graphics g ) 
    {
	//super.paint();
	g.setColor( Color.white );
	FontMetrics fm = getGraphics().getFontMetrics();
	int stringHeight = fm.getHeight();
	g.drawString( text, 20, 8 + (stringHeight/2));
	g.fillRect( 2,2, 13, 13 );

	g.setColor( Color.black );
	g.drawRect( 2,2, 13, 13 );
	g.drawRect( 3,3, 11, 11 );

	g.setColor( darkBlue );
	g.fillRect( 6, 8, 6, 2 );
	
	if (!expanded) {
	    g.fillRect( 8, 6, 2, 6 );
	}
    }
};
