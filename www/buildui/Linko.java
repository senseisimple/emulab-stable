import java.awt.*;
import java.awt.event.*;

class Linko extends Canvas
{
    String text;
    boolean mouseIsOver;
    ActionListener myActionListener;
    private static Color lightBlue;

    static {
	lightBlue = new Color( 0.7f, 0.7f, 1.0f );
    }

    public Linko( String t )
    {
	super();
	setCursor( Cursor.getPredefinedCursor( Cursor.HAND_CURSOR ) );
	text = t;
	mouseIsOver = false;
	myActionListener = null;
	enableEvents( AWTEvent.MOUSE_EVENT_MASK );
    }

    public void addActionListener( ActionListener e ) {
	myActionListener = e;
    }

    protected void processMouseEvent( MouseEvent e ) {
	if (e.getID() == MouseEvent.MOUSE_ENTERED) {
	    mouseIsOver = true;
	} else if (e.getID() == MouseEvent.MOUSE_EXITED) {
	    mouseIsOver = false;
	} else if (e.getID() == MouseEvent.MOUSE_PRESSED) {
	    ActionEvent ce = new ActionEvent( this, 
					      ActionEvent.ACTION_PERFORMED,
					      "clicked" );
	    if (myActionListener != null) {
	      myActionListener.actionPerformed( ce );
	    }
	}
	repaint();
    }

    public Dimension getPreferredSize()
    {
	Graphics g = getGraphics();

	if (g != null) {
	    FontMetrics fm = g.getFontMetrics();
	    if (fm != null) {
		int stringWidth = fm.stringWidth(text);
		return new Dimension( stringWidth, fm.getHeight() + 2 );
	    }
	} 
	
	System.out.println("Crap.");
	
	return new Dimension(200, 40);
    }

    public Dimension getMinimumSize() { return getPreferredSize(); }
    public Dimension getMaximumSize() { return getPreferredSize(); }

    public void paint( Graphics g )
    {
	FontMetrics fm = g.getFontMetrics();
	super.paint(g);

        int stringWidth = fm.stringWidth( text );
	int stringHeight = fm.getHeight();

	Dimension size = getSize();

	int begin = size.width / 2 - stringWidth / 2;
	int end   = size.width / 2 + stringWidth / 2;



	if (mouseIsOver) {

	    g.setColor( Color.white );
	    g.drawLine( begin, stringHeight + 2, end, stringHeight + 2 );
	} else {
	    g.setColor( lightBlue );
	}

	
	g.drawString( text, (size.width / 2) - (stringWidth / 2), fm.getHeight() );
    }   
};
