import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.lang.*;
import java.net.*;
//import java.io.*;

public class Netbuild extends java.applet.Applet 
    implements MouseListener, MouseMotionListener, ActionListener
{
    private WorkArea workArea;
    private Palette  palette;

    private Panel propertiesPanel;

    private PropertiesArea linkPropertiesArea;
    private PropertiesArea lanPropertiesArea;
    private PropertiesArea nodePropertiesArea;
    private PropertiesArea iFacePropertiesArea;
    private PropertiesArea lanLinkPropertiesArea;

    private boolean clickedOnSomething;
    private boolean allowMove;
    private boolean dragStarted;
    private boolean shiftWasDown;
    private boolean selFromPalette;
    private int lastDragX, lastDragY;
    private int downX, downY;

    private static Netbuild me;

    private static Color cornflowerBlue;
    private static Color lightBlue;
    private static Color darkBlue;

    private int workAreaX, workAreaY;
    private int workAreaWidth, workAreaHeight;

    private Linko exportButton;

    static {
	cornflowerBlue  = new Color( 0.95f, 0.95f, 1.0f );
	lightBlue = new Color( 0.9f, 0.9f, 1.0f );
	darkBlue  = new Color( 0.3f, 0.3f, 0.5f );
    }

    private void doittoit( boolean needed, PropertiesArea which, boolean forceExpand ) {
	if (needed) {
	    if (which.isStarted()) {
		which.refresh();
	    } else {
		which.setVisible(false);
		propertiesPanel.add( which );
		which.start();
		if (forceExpand) { 
		    which.showProperties(); 
		} else {
		    which.hideProperties();
		}
		which.setVisible(true);
	    }
	} else {
	    if (which.isStarted()) {
		which.stop();
		propertiesPanel.remove( which );
	    }
	}
    }

    private void startAppropriatePropertiesArea() {
	int typeCount = 0;
	boolean needLanLink = false;
	boolean needLink    = false;
	boolean needLan     = false;
	boolean needIFace   = false;
	boolean needNode    = false;

	Enumeration en = Thingee.selectedElements();
		    
	while(en.hasMoreElements()) {
	    Thingee t = (Thingee)en.nextElement();
	    
	    if (t.propertyEditable) {
		if (t instanceof LanLinkThingee) { 
		    if (!needLanLink) typeCount++; 
		    needLanLink = true; 
		} else if (t instanceof LinkThingee) { 
		    if (!needLink) typeCount++; 
		    needLink = true;
		} else if (t instanceof LanThingee) { 
		    if (!needLan) typeCount++; 
		    needLan = true; 
		} else if (t instanceof IFaceThingee) { 
		    if (!needIFace) typeCount++; 
		    needIFace = true;
		} else if (t instanceof NodeThingee) { 
		    if (!needNode) typeCount++; 
		    needNode = true;
		}
	    }
	}

	boolean exp = typeCount <= 1;
	propertiesPanel.setVisible( false );
	doittoit( needNode, nodePropertiesArea, exp );
	doittoit( needLink, linkPropertiesArea, exp );
	doittoit( needLan,  lanPropertiesArea, exp );
	doittoit( needLanLink, lanLinkPropertiesArea, exp );
	doittoit( needIFace, iFacePropertiesArea, exp );
	propertiesPanel.doLayout();
	propertiesPanel.setVisible( true );

    }

    private boolean isInWorkArea( int x, int y ) {
	return x > workAreaX && y > workAreaY &&
               x < workAreaX + workAreaWidth &&
	       y < workAreaY + workAreaHeight;
    }

    public static void redrawAll() {
	me.repaint();
    }

    public static Image getImage( String name ) {
	System.out.println( me.getCodeBase() );
	System.out.println( name );
	return me.getImage( me.getCodeBase(), name );
    }
	
    public void mouseMoved( MouseEvent e ) {}

    public void mouseDragged( MouseEvent e ) {
	Graphics g = getGraphics();
	g.setXORMode( Color.white );

	if (clickedOnSomething) {
	    if (allowMove) {
		if (dragStarted) {
		    Enumeration en = Thingee.selectedElements();
		    
		    while(en.hasMoreElements()) {
			Thingee t = (Thingee)en.nextElement();
			if (t.moveable || t.trashable) {
			    if (selFromPalette) {
				g.drawRect( t.getX() + lastDragX - 16 , t.getY() + lastDragY - 16, 32, 32 );
			    } else{
				g.drawRect( t.getX() + lastDragX - 16 + workAreaX, 
					    t.getY() + lastDragY - 16 + workAreaY, 
					    32, 32 );
			    }
			}
		    }
		}
		
		dragStarted = true;
		
		lastDragX = e.getX() - downX;
		lastDragY = e.getY() - downY;	
		
		Enumeration en = Thingee.selectedElements();
		
		while(en.hasMoreElements()) {
		    Thingee t = (Thingee)en.nextElement();
		    if (t.moveable || t.trashable) {
			if (selFromPalette) {
			    g.drawRect( t.getX() + lastDragX - 16 , t.getY() + lastDragY - 16, 32, 32 );
			} else{
			    g.drawRect( t.getX() + lastDragX - 16 + workAreaX, 
					t.getY() + lastDragY - 16 + workAreaY, 
					32, 32 );
			}
		    }
		}
	    }
	} else {
	    if (dragStarted) {
		g.drawRect( downX, downY, lastDragX, lastDragY );
	    }
	    dragStarted = true;
	    lastDragX = e.getX() - downX;
	    lastDragY = e.getY() - downY;	
	    g.drawRect( downX, downY, lastDragX, lastDragY );
	}
	g.setPaintMode();
    }

    public void actionPerformed( ActionEvent e ) {
	if (e.getSource() == exportButton) {
	    String ns = workArea.toNS();
	    System.out.println( ns );		
	    String url = getParameter("exporturl") + "?nsdata=" + 
		URLEncoder.encode( ns );
	    System.out.println( url );
	    try {
		getAppletContext().showDocument( new URL( url ), "_blank" );
	    } catch (Exception ex) {
		System.out.println("exception: " + ex.getMessage());
		ex.printStackTrace();	       
	    }
	}
    }

    public void mousePressed( MouseEvent e ) {
	int x = e.getX();
	int y = e.getY();
	/*
	  if (x < 8 && y < 8) {
	
	    try {
	    File foo = new File( "out.txt" );
	    FileOutputStream fos = new FileOutputStream( foo );
	    FilterOutputStream foos = new FilterOutputStream( fos );
	    PrintWriter pw = new PrintWriter( foos );
	    pw.println( workArea.toNS() );
	    pw.flush();
	    } catch (Exception ex ) {
		System.out.println("exception: " + ex.getMessage());
		ex.printStackTrace();	       
		//System.out.println( workArea.toNS());

	    }
	}
*/

	shiftWasDown = e.isShiftDown();
	downX = x;
	downY = y;

	lastDragX = 0;
	lastDragY = 0;

	Thingee clickedOn;

        prePaintSelChange();

	if (isInWorkArea(x,y)) {
	    clickedOn = workArea.clicked( x - 80, y );
	    selFromPalette = false;
	} else { 
	    Thingee.deselectAll();
	    clickedOn = palette.clicked(x, y);
	    selFromPalette = true;
	}

	clickedOnSomething = (clickedOn != null);

	if (e.isControlDown()) {
	    allowMove = false;
	    if (clickedOnSomething) {
		Enumeration en = Thingee.selectedElements();
		
		while(en.hasMoreElements()) {
		    Thingee a = (Thingee)en.nextElement();
		    Thingee b = clickedOn;
		    
		    if (a != b &&
			a != null && b != null &&
			a.linkable && b.linkable) {
			if (a instanceof NodeThingee && b instanceof NodeThingee) {		
			    LinkThingee t = new LinkThingee(Thingee.genName("link"), a, b );
			    workArea.add( t );
			    IFaceThingee it = new IFaceThingee("", a, t );
			    workArea.add( it );
			    IFaceThingee it2 = new IFaceThingee("", b, t );
			    workArea.add( it2 );
			    paintThingee( t );
			    paintThingee( it );
			    paintThingee( it2 );
			} else if (a instanceof NodeThingee && b instanceof LanThingee) {
			    LinkThingee t = new LanLinkThingee("", a, b);
			    workArea.add( t );
			    IFaceThingee it = new IFaceThingee("", a, t );
			    workArea.add( it );
			    paintThingee( t );
			    paintThingee( it );
			} else if (b instanceof NodeThingee && a instanceof LanThingee) {
			    LinkThingee t = new LanLinkThingee("", a, b);
			    workArea.add( t );
			    IFaceThingee it = new IFaceThingee("", b, t );
			    workArea.add( it );
			    paintThingee( t );
			    paintThingee( it );
			}
		    }

		}
	    }
	} else {// if (e.controlDown())
	    allowMove = true;
	    if (clickedOn == null) {
		if (!e.isShiftDown()){
		    Thingee.deselectAll();
		}
	    } else if (clickedOn.isSelected()) {
		if (!e.isShiftDown()) {

		} else {
		    clickedOn.deselect();
		}
	    } else {
		if (!e.isShiftDown()) {
		    Thingee.deselectAll();
		}
		clickedOn.select();
	    }
	}

        paintSelChange();
	startAppropriatePropertiesArea();

	//repaint();

	dragStarted = false;
    }

    private void paintThingee( Thingee t ) {
	Rectangle r = t.getRectangle();
	
	repaint( r.x + workAreaX, r.y, r.width, r.height );
    } 

    private Dictionary wasSelected;
    
    private void prePaintSelChange() {
	wasSelected = new Hashtable();
	Enumeration en = Thingee.selectedElements();
	
	while(en.hasMoreElements()) {	   
	    Thingee t = (Thingee)en.nextElement();
	    wasSelected.put( t, new Integer(1));
	}
    }	

    private void paintSelChange() {
	Enumeration en = Thingee.selectedElements();
	
	while(en.hasMoreElements()) {
	    Thingee t = (Thingee)en.nextElement();

	    if (wasSelected.get(t) == null) {
		paintThingee(t);
		wasSelected.remove(t);
	    }
	}

	en = wasSelected.keys();

	while(en.hasMoreElements()) {
	    Thingee t = (Thingee)en.nextElement();
	    paintThingee(t);
	}
    }

    public void mouseReleased( MouseEvent e ) {
	if (clickedOnSomething) {
	    if (dragStarted) {
		Graphics g = getGraphics();
		g.setXORMode( Color.white );
		
		{
		    Enumeration en = Thingee.selectedElements();
		    
		    while(en.hasMoreElements()) {
			Thingee t = (Thingee)en.nextElement();
			if (t.moveable || t.trashable) {
			    if (selFromPalette) {
				g.drawRect( t.getX() + lastDragX - 16 , t.getY() + lastDragY - 16, 32, 32 );
			    } else{
				g.drawRect( t.getX() + lastDragX - 16 + workAreaX, 
					    t.getY() + lastDragY - 16 + workAreaY, 
					    32, 32 );
			    }
			}
		    }
		}
		
		g.setPaintMode();
		
		int x = e.getX();
		int y = e.getY();
		
		if (selFromPalette) {
		    // from palette..
		    if (x < 80) {
			// back to palette -- nothing happens.
		    } else {
			// into workarea. Create.
			prePaintSelChange();
			Thingee t;
			if (Thingee.selectedElements().nextElement() instanceof NodeThingee) {
			    t = new NodeThingee(Thingee.genName("node"));
			} else {
			    t = new LanThingee(Thingee.genName("lan"));
			}
			t.move( x - workAreaX, y - workAreaY );
			workArea.add( t );
			Thingee.deselectAll();
			t.select();
			selFromPalette = false;
			startAppropriatePropertiesArea();
			//paintThingee(t);
			paintSelChange();
			//repaint();
		    }
		} else {
		    // from workarea..
		    if (!isInWorkArea(x,y)) {
			// out of work area.. but to where?
			if (palette.hitTrash( x, y )) {
			    Enumeration en = Thingee.selectedElements();
			    
			    while(en.hasMoreElements()) {
				Thingee t = (Thingee)en.nextElement();
				if (t.trashable) {
				// into trash -- gone.
				    t.deselect();
				    workArea.remove(t);
				} else if (t instanceof IFaceThingee) {
				    t.deselect();
				}
			    }
			    repaint();
			    startAppropriatePropertiesArea();
			} 
		    } else {
			Enumeration en = Thingee.selectedElements();
			
			while(en.hasMoreElements()) {
			    Thingee t = (Thingee)en.nextElement();
			    
			    if (t.moveable) {
				t.move( t.getX() + lastDragX, t.getY() + lastDragY );
			    }
			    repaint();
			}
		    }
		}
	    }
	} else { // if clickedonsomething
	    // dragrect
	    if (lastDragX > 0 && lastDragY > 0) {
		prePaintSelChange();
		Graphics g = getGraphics();
		g.setXORMode( Color.white );
		if (dragStarted) {
		    g.drawRect( downX, downY, lastDragX, lastDragY );
		}
		g.setPaintMode();
		workArea.selectRectangle( new Rectangle( downX - workAreaX, 
							 downY - workAreaY, 
							 lastDragX, 
							 lastDragY), shiftWasDown );
		paintSelChange();
		startAppropriatePropertiesArea();
	    }
	}
	dragStarted = false;
    }

    public void mouseEntered( MouseEvent e )  {}
    public void mouseExited(  MouseEvent e )  {}
    public void mouseClicked( MouseEvent e )  {}

    public String getAppletInfo() {
	return "Designs a network topology.";
    }

    public Netbuild() {
	me = this;

	setLayout( null );

	//setLayout( new FlowLayout( FlowLayout.RIGHT, 4, 4 ) );
	addMouseListener( this );
	addMouseMotionListener( this );

	workArea = new WorkArea();
	palette  = new Palette();
	propertiesPanel = new Panel();

	nodePropertiesArea  = new NodePropertiesArea();
	linkPropertiesArea  = new LinkPropertiesArea();
	iFacePropertiesArea = new IFacePropertiesArea();
	lanPropertiesArea   = new LanPropertiesArea();
	lanLinkPropertiesArea = new LanLinkPropertiesArea();
	/*
	add( nodePropertiesArea );
	add( lanPropertiesArea );
	add( linkPropertiesArea );
	add( lanLinkPropertiesArea );
	add( iFacePropertiesArea );
	*/
	/*
	propertiesPanel.add( nodePropertiesArea );
	propertiesPanel.add( lanPropertiesArea );
	propertiesPanel.add( linkPropertiesArea );
	propertiesPanel.add( lanLinkPropertiesArea );
	propertiesPanel.add( iFacePropertiesArea );
	*/
	dragStarted = false;
	
	workAreaX = 80;
	workAreaY = 0;
	workAreaWidth = 400;
	workAreaHeight = 479;

	setBackground( darkBlue );
	propertiesPanel.setBackground( darkBlue );
	//propertiesPanel.setBackground( Color.white );
	propertiesPanel.setVisible( true );

	exportButton = new Linko( "[ Export topology ]" );
	exportButton.addActionListener( this );
	//Label l = new Label( "Export topology" );
	
	add( propertiesPanel );

	propertiesPanel.setLocation( workAreaX + workAreaWidth + 8, 0 + 8 );
	//	propertiesPanel.setSize( 640 - 480 - 16, 480 - 16 - 16 );
	propertiesPanel.setSize( 640 - 480 - 16, 480 - 16 - 32 );
	
	exportButton.setVisible( true );
	add( exportButton );
	
	exportButton.setLocation( workAreaX + workAreaWidth + 8, 480 - 24 );
	exportButton.setSize( 640-480-16, exportButton.getPreferredSize().height );	
	
	/*
	nodePropertiesArea.setLocation( workAreaX + workAreaWidth + 8, 0 + 8 );
	nodePropertiesArea.setSize( 640 - 480 - 16, 480 - 16 );
	lanPropertiesArea.setLocation( workAreaX + workAreaWidth + 8, 0 + 8 );
	lanPropertiesArea.setSize( 640 - 480 - 16, 480 - 16 );
	linkPropertiesArea.setLocation( workAreaX + workAreaWidth + 8, 0 + 8 );
	linkPropertiesArea.setSize( 640 - 480 - 16, 480 - 16 );
	lanLinkPropertiesArea.setLocation( workAreaX + workAreaWidth + 8, 0 + 8 );
	lanLinkPropertiesArea.setSize( 640 - 480 - 16, 480 - 16 );
	iFacePropertiesArea.setLocation( workAreaX + workAreaWidth + 8, 0 + 8 );
	iFacePropertiesArea.setSize( 640 - 480 - 16, 480 - 16 );
	*/
    }

    public void paint( Graphics g ) {
	//g.drawLine( 64,0,63,479 );
	//g.fillRect( 0,0, 64, 480 );

	g.setColor( lightBlue );
	g.fillRect( 0, 0, workAreaX, 479 );

	g.setColor( cornflowerBlue );
	g.fillRect( workAreaX, 0, workAreaWidth, 479 );

	g.setColor( darkBlue );
	g.fillRect( workAreaX + workAreaWidth, 0, 
		    (639 - workAreaX - workAreaWidth), 479 );

	g.setColor( Color.black );
	g.drawRect( 0, 0, workAreaX, 479 );
	g.drawRect( workAreaX, 0, workAreaWidth, 479 );
	g.drawRect( workAreaX + workAreaWidth, 0, 
		    (639 - workAreaX - workAreaWidth), 479 );
	g.drawString( "NetBuild v0.0.1", workAreaX + 4, 474 );

	palette.paint( g );
      	//propertiesArea.paint( g );

	g.translate( workAreaX, workAreaY );
	g.setClip( 1, 1, workAreaWidth - 1, workAreaHeight - 1 );
	workArea.paint( g );
    }
}
