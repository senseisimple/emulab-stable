/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

import java.awt.*;
import java.awt.event.*;
import java.awt.image.ImageObserver;
import java.awt.image.BufferedImage;
import javax.swing.*;
import javax.swing.event.*;
import javax.swing.table.AbstractTableModel;
import java.net.URL;
import java.lang.Math;
import java.util.*;
import java.text.*;
import java.io.*;
import java.net.URL;
import java.net.URLEncoder;
import java.net.URLConnection;
import java.text.DecimalFormat;

/*
 * Draw the floormap and put little dots on it. 
 */
public class RoboTrack extends JApplet {
    static Map map;
    static JTable maptable;
    static JPopupMenu MenuPopup;
    static JMenuItem CancelMenuItem, SubmitMenuItem;
    static Image floorimage;
    static double pixels_per_meter = 1.0;
    boolean frozen = false;
    static final DecimalFormat FORMATTER = new DecimalFormat("0.00");
    String uid, auth;
    boolean shelled = false;
    
    /*
     * The connection to boss that will provide robot location info.
     * When run interactively (from main) it is set to stdin (see below).
     */
    static InputStream is;

    public void init() {
	try
	{
	    URL urlServer = this.getCodeBase(), robopipe, floorurl;
	    String pipeurl, baseurl;
	    URLConnection uc;

	    /* Get our parameters then */
	    uid     = this.getParameter("uid");
	    auth    = this.getParameter("auth");
	    pipeurl = this.getParameter("pipeurl");
	    baseurl = this.getParameter("baseurl");
	    pixels_per_meter = Double.parseDouble(this.getParameter("ppm"));

	    // form the URL that we use to get the background image
	    floorurl = new URL(urlServer,
			       baseurl
			       + "&nocookieuid="
			       + URLEncoder.encode(uid)
			       + "&nocookieauth="
			       + URLEncoder.encode(auth));

	    // System.out.println(urlServer.toString());
	    // System.out.println(floorurl.toString());

	    // and then get it.
	    floorimage = getImage(floorurl);

	    /* ... form the URL that we will connect to. */
	    robopipe = new URL(urlServer,
			       pipeurl
			       + "&nocookieuid="
			       + URLEncoder.encode(uid)
			       + "&nocookieauth="
			       + URLEncoder.encode(auth));

	    uc = robopipe.openConnection();
	    uc.setDoInput(true);
	    uc.setUseCaches(false);
	    this.is = uc.getInputStream();
	}
	catch(Throwable th)
	{
	    th.printStackTrace();
	}

	/*
	 * Mouse listener. See below.
	 */
	MyMouseAdaptor mymouser = new MyMouseAdaptor();
	addMouseListener(mymouser);
	addMouseMotionListener(mymouser);

	/*
	 * Make sure the redraw stops when the window is iconified.
	 * Hmm, not sure how to do this yet. How do I get a handle
	 * on the current frame?
	addWindowListener(new WindowAdapter() {
		public void windowIconified(WindowEvent e) { stop(); }
		public void windowDeiconified(WindowEvent e) { start(); }
	    });	
	 */
	
	/*
	 * Vertical placement of Components in the pane. 
	 */
	getContentPane().setLayout(new BoxLayout(getContentPane(),
						 BoxLayout.PAGE_AXIS));
        getContentPane().add(map = new Map());

	/*
	 * This sillyness is how you get the column headers to show
	 * when you do not put the Jtable inside a scroll thingy.
	 */
	maptable = new JTable(new MyTableModel());
	getContentPane().add(maptable.getTableHeader(), BorderLayout.NORTH);
	getContentPane().add(maptable, BorderLayout.CENTER);

	/*
	 * Create the popup menu that will be used to fire off
	 * robot moves. We use the mouseadaptor for this too. 
	 */
        MenuPopup = new JPopupMenu("dd");
        SubmitMenuItem = new JMenuItem("Submit All Moves");
        SubmitMenuItem.addActionListener(mymouser);
        MenuPopup.add(SubmitMenuItem);
        CancelMenuItem = new JMenuItem("Cancel All Moves");
        CancelMenuItem.addActionListener(mymouser);
        MenuPopup.add(CancelMenuItem);
        JMenuItem menuitem = new JMenuItem("Close This Menu");
        menuitem.addActionListener(mymouser);
        MenuPopup.add(menuitem);
	MenuPopup.setBorder(BorderFactory.createLineBorder(Color.black));
    }

    public void start() {
        map.start();
    }
  
    public void stop() {
        map.stop();
    }

    /*
     * A thing that waits for an image to be loaded. 
     */
    public Image getImage(URL url) {
	Image img = getToolkit().getImage(url);
	try {
	    MediaTracker tracker = new MediaTracker(this);
	    tracker.addImage(img, 0);
	    tracker.waitForID(0);
	} catch (Exception e) {}
	return img;
    }

    /*
     * A container for coordinate information.
     */
    static class Robot {
        int x, y;			// Current x,y coords in pixels
	double z;			// Current z in meters
	double or = 500.0;		// Current orientation 
	int dx, dy;			// Destination x,y coords in pixels
	double dor = 500.0;		// Destination orientation
	boolean gotdest = false;	// We have a valid destination
	boolean dragging = false;	// Drag and Drop in progress.
	int drag_x, drag_y;		// Current drag x,y coords.
	double drag_or = 500.0;		// Drag orientation
	
	/*
	 * These are formatted as strings to avoid doing conversons
	 * on the fly when the table is redrawn. Note, we have to
	 * convert from pixel coords (what the server sends) to meters.
	 */
	String battery_voltage    = "";
	String battery_percentage = "";
	String x_meters           = "";
	String y_meters           = "";
	String z_meters           = "";
	String or_string          = "";
	String dx_meters          = "";
	String dy_meters          = "";
	String dor_string         = "";
	String dragx_meters       = "";
	String dragy_meters       = "";
	String dragor_string      = "";
	String pname, vname;
	long last_update          = 0;	// Unix time of last update.
	String update_string      = "";
	int index;
    }
    // Indexed by the robot physical name, points Robot struct above.
    static Dictionary robots     = new Hashtable();
    // Map from integer index to a Robot struct.
    static Vector     robotmap   = new Vector(10, 10);;
    static int	      robotcount = 0;

    static class Map extends JPanel implements Runnable {
        private Thread thread;
        private BufferedImage bimg;
	private Graphics2D G2 = null;

	// The DOT radius.
	int DOT_RAD   = 20;
	// The length of the orientation stick, from the center of the circle.
	int STICK_LEN = 20;
	// The distance from center of dot to the label x,y.
	int LABEL_X   = -20;
	int LABEL_Y   = 20;
	
        public Map() {
        }

	/*
	 * Compute the endpoint of a line segment starting at the
	 * center of a circle (x,y) with orientation (angle).
	 */
	public int[] ComputeOrientationLine(int x, int y,
					    int dist, double angle) {
	    int retvals[] = new int[2];

	    retvals[0] = (int) (x + dist *
				Math.cos(-angle * 3.1415926536 / 180.0));
	    retvals[1] = (int) (y + dist *
				Math.sin(-angle * 3.1415926536 / 180.0));
    
	    return retvals;	    
	}

	/*
	 * I stole this from some demo programs. Not really sure what
	 * it does exactly, but has something to do with double buffering.
	 */
	public Graphics2D createGraphics2D(int w, int h) {
	    //System.out.println("createGraphics2D");

	    if (G2 == null) {
		bimg = (BufferedImage) createImage(w, h);
		
		G2 = bimg.createGraphics();
		G2.setBackground(getBackground());
		G2.setRenderingHint(RenderingHints.KEY_RENDERING,
				    RenderingHints.VALUE_RENDER_QUALITY);
	    }
	    G2.clearRect(0, 0, w, h);
	    return G2;
	}

	/*
	 * Parse the data returned by the web server. This is bogus; we
	 * should create an XML representation of it!
	 */
	public void parseRobot(String str) {
	    StringTokenizer tokens = new StringTokenizer(str, ",");
	    Calendar Now = new GregorianCalendar();
	    String tmp;
	    Robot robbie;
	    int index;

	    System.out.println(str);

	    tmp = tokens.nextToken().trim();	    

	    if ((robbie = (Robot) robots.get(tmp)) == null) {
		robbie = new Robot();
		index  = robotcount++;
		robbie.index = index;
		robbie.pname = tmp;
		robots.put(tmp, robbie);
		robotmap.insertElementAt(robbie, index);
	    }
	    robbie.vname = tokens.nextToken().trim();
	    robbie.x     = Integer.parseInt(tokens.nextToken().trim());
	    robbie.y     = Integer.parseInt(tokens.nextToken().trim());
	    robbie.x_meters = FORMATTER.format(robbie.x / pixels_per_meter);
	    robbie.y_meters = FORMATTER.format(robbie.y / pixels_per_meter);

	    str = tokens.nextToken().trim();
	    if (str.length() > 0) {
		robbie.z = Double.parseDouble(str);
		robbie.z_meters = FORMATTER.format(robbie.z);
	    }
	    else {
		robbie.z        = 0.0;
		robbie.z_meters = "";
	    }

	    str = tokens.nextToken().trim();
	    if (str.length() > 0) {
		robbie.or = Double.parseDouble(str);
		robbie.or_string = FORMATTER.format(robbie.or);
	    }
	    else {
		robbie.or        = 500.0;
		robbie.or_string = "";
	    }

	    str = tokens.nextToken().trim();
	    if (str.length() > 0) {
		robbie.dx  = Integer.parseInt(str);
		robbie.dy  = Integer.parseInt(tokens.nextToken().trim());
		robbie.dor = Double.parseDouble(tokens.nextToken().trim());
		robbie.dx_meters  = FORMATTER.format(robbie.dx /
						    pixels_per_meter);
		robbie.dy_meters  = FORMATTER.format(robbie.dy /
						    pixels_per_meter);
		robbie.dor_string = FORMATTER.format(robbie.dor);
		robbie.gotdest = true;
	    }
	    else {
		// Consume next two tokens and clear strings/values.
		str = tokens.nextToken();
		str = tokens.nextToken();
		robbie.dx  = 0;
		robbie.dy  = 0;
		robbie.dor = 500.0;
		robbie.dx_meters  = "";
		robbie.dy_meters  = "";
		robbie.dor_string = "";
		robbie.gotdest = false;
	    }

	    // Store these as strings for easy display.
	    str = tokens.nextToken().trim();
	    if (str.length() > 0)
		robbie.battery_percentage =
		    FORMATTER.format(Float.parseFloat(str));
	    else
		robbie.battery_percentage = "";

	    str = tokens.nextToken().trim();
	    if (str.length() > 0)
		robbie.battery_voltage = 
		    FORMATTER.format(Float.parseFloat(str));
	    else
		robbie.battery_voltage = "";

	    robbie.last_update   = Now.getTimeInMillis();
	    robbie.update_string = Now.get(Calendar.HOUR_OF_DAY) + ":" +
		Now.get(Calendar.MINUTE) + ":" + Now.get(Calendar.SECOND);
	}

	/*
	 * Given an x,y from a button click, try to map the coords
	 * to a robot that has been drawn on the screen. There are
	 * probably race conditions here, but I doubt they will cause
	 * any real problems (like, maybe we pick the wrong robot).
	 */
	public String pickRobot(int x, int y) {
	    Enumeration e = robots.elements();

	    while (e.hasMoreElements()) {
		Robot robbie  = (Robot)e.nextElement();

		if ((Math.abs(robbie.y - y) < 10 &&
		     Math.abs(robbie.x - x) < 10) ||
		    (robbie.dragging &&
		     Math.abs(robbie.drag_y - y) < 10 &&
		     Math.abs(robbie.drag_x - x) < 10)) {
		    return robbie.pname;
		}
	    }
	    return "";
	}

	/*
	 * Draw some buttons.
	 */

	/*
	 * Draw a robot, which is either the real one, a destination one,
	 * or a shadow one being dragged around.
	 */
	static int DRAWROBOT_LOC  = 1;
	static int DRAWROBOT_DST  = 2;
	static int DRAWROBOT_DRAG = 3;
	
	public void drawRobot(Graphics2D g2,
			      int x, int y, double or,
			      String label, int which) {
	    /*
	     * An allocated robot is a filled circle.
	     * A destination is an unfilled circle. So is a dragging robot.
	     */
	    if (which == DRAWROBOT_LOC) {
		g2.setColor(Color.green);
		g2.fillOval(x - (DOT_RAD/2), y - (DOT_RAD/2),
			    DOT_RAD, DOT_RAD);
	    }
	    else if (which == DRAWROBOT_DST) {
		g2.setColor(Color.green);
		g2.drawOval(x - (DOT_RAD/2), y - (DOT_RAD/2),
			    DOT_RAD, DOT_RAD);
	    }
	    else if (which == DRAWROBOT_DRAG) {
		g2.setColor(Color.red);
		g2.drawOval(x - (DOT_RAD/2), y - (DOT_RAD/2),
			    DOT_RAD, DOT_RAD);
	    }
	    
	    /*
	     * If there is a orientation, add an orientation stick
	     * to it.
	     */
	    if (or != 500.0) {
		int endpoint[] = ComputeOrientationLine(x, y, STICK_LEN, or);

		//System.out.println(or + " " + x + " " + y + " " +
		//                   endpoint[0] + " " + endpoint[1]);
		
		g2.drawLine(x, y, endpoint[0], endpoint[1]);
	    }

	    /*
	     * Draw a label for the robot, either above or below,
	     * depending on where the orientation stick was.
	     */
	    int lx  = x + LABEL_X;
	    int ly  = y + LABEL_Y;
	    int dlx = x + LABEL_X - 15;
	    int dly = ly + 12;

	    if ((or > 180.0 && or < 360.0) ||
		(or < 0.0   && or > -180.0)) {
		ly  = y - (LABEL_Y/2);
		dly = ly - (LABEL_Y/2);
	    }

	    g2.setColor(Color.black);
	    g2.drawString(label, lx, ly);

	    /*
	     * If dragging, then put in the drag coordinates (in meters).
	     */
	    if (which == DRAWROBOT_DRAG) {
		String text =
		    "(" + FORMATTER.format(x / pixels_per_meter) + ","
		        + FORMATTER.format(y / pixels_per_meter) + ")";

		g2.drawString(text, dlx, dly);
	    }
	}

	public void drawMap(int w, int h, Graphics2D g2) {
	    int fw, fh;
	    
	    fw = floorimage.getWidth(this);
	    fh = floorimage.getHeight(this);

	    /*
	     * The base image is the floormap.
	     */
	    g2.drawImage(floorimage, 0, 0, fw, fh, this);

	    /*
	     * Then we draw a bunch of stuff on it, like the robots.
	     */
	    Enumeration e = robots.elements();

	    while (e.hasMoreElements()) {
		Robot robbie  = (Robot)e.nextElement();

		int x = robbie.x;
		int y = robbie.y;

		drawRobot(g2, x, y, robbie.or, robbie.pname, DRAWROBOT_LOC);

		/*
		 * Okay, if the robot has a destination, draw that too
		 * but as a hollow circle.
		 */
		if (robbie.gotdest) {
		    int dx = robbie.dx;
		    int dy = robbie.dy;
		    
		    drawRobot(g2, dx, dy, robbie.dor, robbie.pname,
			      DRAWROBOT_DST);

		    /*
		     * And draw a light grey line from source to dest.
		     */
		    g2.setColor(Color.gray);
		    g2.drawLine(x, y, dx, dy);
		}
		else if (robbie.dragging) {
		    int dx = robbie.drag_x;
		    int dy = robbie.drag_y;
		    
		    drawRobot(g2, dx, dy, robbie.drag_or, robbie.pname,
			      DRAWROBOT_DRAG);

		    /*
		     * And draw a red line from source to drag.
		     */
		    g2.setColor(Color.red);
		    g2.drawLine(x, y, dx, dy);
		}
	    }
	}

	/*
	 * This is the callback to paint the map. 
	 */
        public void paint(Graphics g) {
            Dimension dim  = getSize();
	    Rectangle rect = map.getBounds();
	    int h = dim.height;
	    int w = dim.width;

	    //System.out.println(w + " D " + h);
	    //System.out.println(rect.width + " R " + rect.height);
	    
            Graphics2D g2 = createGraphics2D(w, h);

            drawMap(w, h, g2);
            g.drawImage(bimg, 0, 0, this);
        }
    
        public void start() {
	    G2     = null;
            thread = new Thread(this);
            thread.start();
        }
    
        public synchronized void stop() {
            thread = null;
        }
    
        public void run() {
            Thread me = Thread.currentThread();
	    byte buffer[] = new byte[6];
	    BufferedReader input
		= new BufferedReader(new InputStreamReader(is));
	    String str;
	    long start_time = System.currentTimeMillis();
	    
            while (thread == me) {
		try
		{
		    while (null != ((str = input.readLine()))) {
			long now  = System.currentTimeMillis();
			long diff = (now - start_time) / 1000;

			System.out.println("" + diff);
			
			parseRobot(str);
			repaint();
			maptable.repaint(10);
			if (thread == null)
			    break;
		    }
		}
		catch(IOException e)
		{
		    e.printStackTrace();
		    break;
		}
                try {
                    thread.sleep(1000);
                }
		catch (InterruptedException e)
		{
		    break;
		}
            }
            thread = null;
        }

	/*
	 * Catch termination.
	 */
	public void destroy() {
	    try
	    {
	        if (is != null)
		    is.close();
	    }
	    catch(IOException e)
	    {
		e.printStackTrace();
	    }
	}
    }
    
    public class MyTableModel extends AbstractTableModel {
        private String[] columnNames = {"Pname", "Vname",
	                                "X (meters)", "Y (meters)",
					"Z (meters)",
					"O (degrees)",
	                                "Dest-X", "Dest-Y", "Dest-O",
					"Battery %", "Voltage", "Updated"
	};
	
        public int getColumnCount() {
            return columnNames.length;
        }

        public int getRowCount() {
	    //	    System.out.println("foo");
	    /*
	     * Hmm, if I return 0 (as before there are any robots) the
	     * table is never resized up. I ma sure there is a way to
	     * deal with this ... not sure what it is yet. So hardwire
	     * table size for now.
	     */
	    // return robots.size();
	    return 8;
        }

        public String getColumnName(int col) {
            return columnNames[col];
        }

        public Object getValueAt(int row, int col) {
	    if (robots.size() == 0 || row >= robots.size())
		return "";
	    
	    Robot robbie = (Robot) robotmap.elementAt(row);

	    //	    System.out.println(robbie.pname);

	    switch (col) {
	    case 0: return robbie.pname;
	    case 1: return robbie.vname;
	    case 2: return robbie.x_meters;
	    case 3: return robbie.y_meters;
	    case 4: return robbie.z_meters;
	    case 5: return robbie.or_string;
	    case 6: return (robbie.dragging ?
			    robbie.dragx_meters : robbie.dx_meters);
	    case 7: return (robbie.dragging ?
			    robbie.dragy_meters : robbie.dy_meters);
	    case 8: return (robbie.dragging ?
			    robbie.dragor_string : robbie.dor_string);
	    case 9: return robbie.battery_percentage;
	    case 10: return robbie.battery_voltage;
	    case 11: return robbie.update_string;
	    }
	    return "Foo";
        }

       /*
	* JTable uses this method to determine the default renderer/
	* editor for each cell.  If we didn't implement this method,
	* then the last column would contain text ("true"/"false"),
	* rather than a check box.
	*/
        public Class getColumnClass(int col) {
	    return String.class;
        }

        /*
         * Dest x,y,o are editable only when the robot is being dragged.
	 * The destination orientation is editable when being dragged *or*
	 * anytime there is no current destination.
         */
        public boolean isCellEditable(int row, int col) {
	    if (robots.size() == 0 || row >= robots.size())
		return false;
	    
	    Robot robbie = (Robot) robotmap.elementAt(row);

	    if ((robbie.dragging && (col >= 6 && col <= 8)) ||
		(!robbie.gotdest && col == 8))
		return true;
	    return false;
        }

	/*
	 * As above, allow changes for a dragging robot. 
	 */
	public void setValueAt(Object value, int row, int col) {
	    if (robots.size() == 0 || row >= robots.size())
		return;
	    
	    Robot robbie = (Robot) robotmap.elementAt(row);

	    if ((robbie.dragging && (col >= 6 && col <= 8)) ||
		(!robbie.gotdest && col == 8)) {
		String stmp = value.toString().trim();
		double dtmp = Double.parseDouble(stmp);
		
		switch (col) {
		case 6:
		    robbie.drag_x = (int) (dtmp * pixels_per_meter);
		    robbie.dragx_meters = stmp;
		    break;
		    
		case 7:
		    robbie.drag_y = (int) (dtmp * pixels_per_meter);
		    robbie.dragy_meters = stmp;
		    break;
		    
		case 8: 
		    robbie.drag_or = dtmp;
		    robbie.dragor_string = stmp;
		    break;
		}
		/*
		 * If the change(s) left the robot within a couple of
		 * pixels of its current location, then cancel this
		 * drag operation. There is probably a better UI for
		 * this.
		 *
		 * XXX I assume that the user will not be holding down the
		 * left mouse button *and* editing a row in the table
		 * (at the same time).
		 */
		if (Math.abs(robbie.drag_x - robbie.x) <= 1 &&
		    Math.abs(robbie.drag_y - robbie.y) <= 1 &&
		    robbie.drag_or == robbie.or) {
		    robbie.dragging = false;
		}
		else if (!robbie.dragging) {
		    /*
		     * User changed the orientation, but the robot was not
		     * currenly being dragged, so make sure things are
		     * initialized properly for a drag.
		     */
		    robbie.drag_x   = robbie.x;
		    robbie.drag_y   = robbie.y;
		    robbie.dragx_meters = robbie.x_meters;
		    robbie.dragy_meters = robbie.y_meters;
		    robbie.dragging = true;
		}
		fireTableCellUpdated(row, col);
		repaint();
		return;
	    }
	}
    }

    /*
     * Mouse button event handler.
     */
    public class MyMouseAdaptor implements MouseInputListener, ActionListener {
	String node_id   = null;
	int click_x      = 0;
	int click_y      = 0;
	boolean dragging = false;
	boolean clicked  = false;
	
	public void mousePressed(MouseEvent e) {
	    int button = e.getButton();

	    if (button == e.BUTTON1) {
		/*
		 * Left button pressed. This starts a drag operation.
		 */
		if (!dragging && !clicked) {
		    node_id = map.pickRobot(e.getX(), e.getY());

		    if (node_id == "") {
			node_id = null;
			MaybeShowMenu(e);
			return;
		    }
		    Robot robbie = (Robot) robots.get(node_id);
		    
		    robbie.drag_x   = e.getX();
		    robbie.drag_y   = e.getY();

		    /*
		     * If already dragging, then leave the dest orientation
		     * as it was. Otherwise, initialize it to the current
		     * orientation.
		     */
		    if (!robbie.dragging) {
			robbie.drag_or  = robbie.dor;
			// For the table.
			robbie.dragor_string = robbie.or_string;
		    }
		    robbie.dragging = true;

		    // For the table.
		    robbie.dragx_meters =
			FORMATTER.format(robbie.drag_x / pixels_per_meter);
		    robbie.dragy_meters =
			FORMATTER.format(robbie.drag_y / pixels_per_meter);
		    
		    dragging = true;
		    repaint();

		    maptable.setRowSelectionInterval(robbie.index,
						     robbie.index);
		    maptable.editCellAt(robbie.index, 8);
		}
	    }
	    else if (button == e.BUTTON2) {
		/*
		 * Middle mouse button starts and stop the applet.
		 */
		if (frozen) {
		    frozen = false;
		    System.out.println("Restarting applet ...");
		    start();
		}
		else {
		    frozen = true;
		    System.out.println("Stopping applet ...");
		    stop();
		}
	    }
	    else if (button == e.BUTTON3) {
		/*
		 * Right mouse button will bring up a showpage on release.
		 * Eventually this will be a context menu.
		 */
		if (! dragging) {
		    node_id = map.pickRobot(e.getX(), e.getY());

		    if (node_id == "") {
			node_id = null;
			return;
		    }
		    
		    Robot robbie = (Robot) robots.get(node_id);

		    click_x = e.getX();
		    click_y = e.getY();
		    clicked = true;
		}
	    }
	}
	
	public void mouseReleased(MouseEvent e) {
	    int button = e.getButton();

	    if (button == e.BUTTON1) {
		if (dragging) {
		    /*
		     * Left button released. This terminates the dragging
		     * operation. The robot is left with the final coords.
		     */
		    // Make sure we received the down event.
		    if (node_id == null) {
			clicked = false;
			return;
		    }
		    System.out.println("Drag finished: " + node_id);
		    
		    Robot robbie = (Robot) robots.get(node_id);

		    robbie.drag_x = e.getX();
		    robbie.drag_y = e.getY();
		    
		    // For the table.
		    robbie.dragx_meters =
			FORMATTER.format(robbie.drag_x / pixels_per_meter);
		    robbie.dragy_meters =
			FORMATTER.format(robbie.drag_y / pixels_per_meter);
		    
		    dragging      = false;
		    node_id       = null;

		    /*
		     * If the release left the robot within a couple of
		     * pixels of its current location, then cancel this
		     * drag operation. There is a probably a better UI
		     * for this. 
		     */
		    if (Math.abs(e.getX() - robbie.x) <= 2 &&
			Math.abs(e.getY() - robbie.y) <= 2 &&
			robbie.drag_or == robbie.or) {
			robbie.dragging = false;
		    }
		    repaint();
		}
	    }
	    else if (button == e.BUTTON3) {
		/*
		 * Right mouse button release brings up a showpage.
		 * Eventually this will be a context menu.
		 */
		if (clicked) {
		    // Make sure we received the down event.
		    if (node_id == null) {
			clicked = false;
			return;
		    }
		    System.out.println("Click finished: " + node_id);

		    Robot robbie = (Robot) robots.get(node_id);
		    node_id      = null;

		    if (! shelled) {
			// This will fail when I run it from the shell
			try
			{
			    URL url = new URL(getCodeBase(),
					      "/shownode.php3?node_id=" +
					        robbie.pname
					      + "&nocookieuid="
					      + URLEncoder.encode(uid)
					      + "&nocookieauth="
					      + URLEncoder.encode(auth));
			    System.out.println(url.toString());
			    getAppletContext().showDocument(url, "_robbie");
			}
			catch(Throwable th)
			{
			    th.printStackTrace();
			}
		    }
		}
		// Clear the click.
		clicked = false;
	    }
	}

	public void mouseEntered(MouseEvent e) {
	}

	/*
	 * If we leave the component, cancel the whole operation.
	 */
	public void mouseExited(MouseEvent e) {
	    // Make sure we are actually dragging/clicking a robot.
	    if (node_id == null) {
		return;
	    }
	    if (dragging) {
		Robot robbie = (Robot) robots.get(node_id);

		clicked = dragging = false;
		robbie.dragging    = false;
		repaint();
		maptable.repaint(10);
	    }
	}

	public void mouseClicked(MouseEvent e) {
	}

	/*
	 * Moving the mouse while clicked is a "drag" event.
	 */
	public void mouseDragged(MouseEvent e) {
	    // Make sure we are actually dragging/clicking a robot.
	    if (node_id == null) {
		return;
	    }

	    if (dragging) {
		Robot robbie = (Robot) robots.get(node_id);

		/*
		 * Update the current drag location. If the robot has moved
		 * more then a few pixels, update its location and force the
		 * map to be redrawn.
		 */
		int old_x = robbie.drag_x;
		int old_y = robbie.drag_y;
		int new_x = e.getX();
		int new_y = e.getY();

		if (Math.abs(new_x - old_x) >= 3 ||
		    Math.abs(new_y - old_y) >= 3) {
		    robbie.drag_x = new_x;
		    robbie.drag_y = new_y;
		    
		    // For the table.
		    robbie.dragx_meters =
			FORMATTER.format(new_x / pixels_per_meter);
		    robbie.dragy_meters =
			FORMATTER.format(new_y / pixels_per_meter);
		    
		    repaint();
		    maptable.repaint(10);
		}
	    }
	}

	public void mouseMoved(MouseEvent e) {
	}


	/*
	 * Decide if we want to actually show the popupmenu; only if
	 * there are robots involved in a drag. Scan the list.
	 */
	public void MaybeShowMenu(MouseEvent e) {
	    Enumeration enum = robots.elements();

	    while (enum.hasMoreElements()) {
		Robot robbie  = (Robot)enum.nextElement();

		if (robbie.dragging) {
		    /*
		     * Use "map" (us) as the invoking component so that the 
		     * menu is fully contained within the applet; this avoids
		     * the "Java Applet Window" warning so that tells people
		     * a window has just been popped up.
		     */
		    MenuPopup.show(map, e.getX(), e.getY());
		    return;
		}
	    }
	}

	/*
	 * This is for the popup menu.
	 */
	public void actionPerformed(ActionEvent e) {
	    JMenuItem source = (JMenuItem)(e.getSource());
	    String    choice = source.getText();
	    
	    System.out.println("Popup select: " + choice + "\n");

	    /*
	     * Figuring out which item was selected is silly.
	     */
	    if (source == CancelMenuItem) {
		/*
		 * Clear all the dragging bits.
		 */
		Enumeration enum = robots.elements();

		while (enum.hasMoreElements()) {
		    Robot robbie  = (Robot)enum.nextElement();

		    robbie.dragging = false;
		}
		maptable.clearSelection();
	    }
	    else if (source == SubmitMenuItem) {
		SendInDestinations();
	    }
	    repaint();
	    maptable.repaint(10);
	}
    }

    /*
     * Send all destinations into the web server.
     */
    public boolean SendInDestinations() {
	Enumeration e = robots.elements();

	// Its a POST, so no leading "?" for the URL arguments.
 	String urlstring = "fromapplet=1"
	    + "nocookieuid="
	    + URLEncoder.encode(uid)
	    + "&nocookieauth="
	    + URLEncoder.encode(auth);

	while (e.hasMoreElements()) {
	    Robot robbie  = (Robot)e.nextElement();

	    if (robbie.dragging) {
		urlstring = urlstring +
		    "&nodeidlist[" + robbie.pname + "]=\"" +
		    robbie.drag_x / pixels_per_meter + "," +
		    robbie.drag_y / pixels_per_meter + "," +
		    robbie.drag_or + "\"";

		// Clear it in the map (when it repaints).
		robbie.dragging = false;
	    }
	}
	if (shelled) {
	    System.out.println(urlstring);
	    return true;
	}

	// This will fail when I run it from the shell
	try
	{
	    URL			url = new URL(getCodeBase(), "setdest.php3");
	    URLConnection	urlConn;
	    DataOutputStream    printout;
	    BufferedReader      input;
	    String		str = "", tmp;
	    
	    System.out.println(url.toString());
	    System.out.println(urlstring);

	    /*
	     * All of this copied from Chads Netbuild code.
	     */
	    // URL connection channel.
	    urlConn = url.openConnection();
	    // Let the run-time system (RTS) know that we want input.
	    urlConn.setDoInput(true);
	    // Let the RTS know that we want to do output.
	    urlConn.setDoOutput(true);
	    // No caching, we want the real thing.
	    urlConn.setUseCaches(false);
	    // Specify the content type.
	    urlConn.setRequestProperty("Content-Type", 
				       "application/x-www-form-urlencoded");
	    // Send POST output.
	    printout = new DataOutputStream(urlConn.getOutputStream ());
	    printout.writeBytes(urlstring);
	    printout.flush();
	    printout.close();
	    // Get response data.
	    input = new BufferedReader(new
		InputStreamReader(urlConn.getInputStream()));
	    while (null != ((tmp = input.readLine()))) {
		str = str + "\n" + tmp;
	    }
	    input.close();

	    if (str.length() > 0) {
		System.out.println(str);

		JOptionPane.showMessageDialog(getContentPane(),
					      str, "Submit Failed",
					      JOptionPane.ERROR_MESSAGE);
	    }
	}
	catch(Throwable th)
	{
	    th.printStackTrace();
	}
	return true;
    }

    public static void main(String argv[]) {
        final RoboTrack robomap = new RoboTrack();
	try
	{
	    URL url = new URL("file://robots-4.jpg");
	    robomap.init();
	    robomap.is = System.in;
	    robomap.floorimage = robomap.getImage(url);
	    robomap.shelled = true;
	    robomap.uid = "stoller";
	    robomap.auth = "xyz";
	}
	catch(Throwable th)
	{
	    th.printStackTrace();
	}
	
        Frame f = new Frame("Robot Map");
	
        f.addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent e) {System.exit(0);}
            public void windowDeiconified(WindowEvent e) { robomap.start(); }
            public void windowIconified(WindowEvent e) { robomap.stop(); }
        });
        f.add(robomap);
        f.pack();
        f.setSize(new Dimension(900,600));
        f.show();
        robomap.start();
    }
}
