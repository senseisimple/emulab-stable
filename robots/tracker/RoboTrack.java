/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

import java.awt.*;
import java.awt.event.*;
import java.awt.image.ImageObserver;
import java.awt.image.BufferedImage;
import java.awt.font.*;
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
    Map map;
    JTable maptable;
    JPopupMenu LeftMenuPopup, RightMenuPopup;
    JMenuItem CancelMovesMenuItem, SubmitMenuItem;
    JMenuItem CancelDragMenuItem, ShowNodeMenuItem, SetOrientationMenuItem;
    Image floorimage;
    double pixels_per_meter = 1.0;
    boolean frozen = false;
    static final DecimalFormat FORMATTER = new DecimalFormat("0.00");
    static final SimpleDateFormat TIME_FORMAT =
	new SimpleDateFormat("hh:mm:ss");
    static final Date now = new Date();
    String uid, auth, building, floor;
    boolean shelled = false;
    
    /*
     * The connection to boss that will provide robot location info.
     * When run interactively (from main) it is set to stdin (see below).
     */
    InputStream is;

    public void init() {
	try
	{
	    URL urlServer = this.getCodeBase(), robopipe, floorurl;
	    String pipeurl, baseurl;
	    URLConnection uc;

	    /* Get our parameters then */
	    uid      = this.getParameter("uid");
	    auth     = this.getParameter("auth");
	    pipeurl  = this.getParameter("pipeurl");
	    baseurl  = this.getParameter("floorurl");
	    building = this.getParameter("building");
	    floor    = this.getParameter("floor");
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
	 * Make sure the redraw stops when the window is iconified.
	 * Hmm, not sure how to do this yet. How do I get a handle
	 * on the current frame?
	addWindowListener(new WindowAdapter() {
		public void windowIconified(WindowEvent e) { stop(); }
		public void windowDeiconified(WindowEvent e) { start(); }
	    });	
	 */

	// This is used below for the listeners. 
	MyMouseAdaptor mymouser = new MyMouseAdaptor();
	
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
        LeftMenuPopup = new JPopupMenu("Tracker");
        JMenuItem menuitem = new JMenuItem("   Motion Menu   ");
        LeftMenuPopup.add(menuitem);
	LeftMenuPopup.addSeparator();
        SubmitMenuItem = new JMenuItem("Submit All Moves");
        SubmitMenuItem.addActionListener(mymouser);
        LeftMenuPopup.add(SubmitMenuItem);
        CancelMovesMenuItem = new JMenuItem("Cancel All Moves");
        CancelMovesMenuItem.addActionListener(mymouser);
        LeftMenuPopup.add(CancelMovesMenuItem);
        menuitem = new JMenuItem("Close This Menu");
        menuitem.addActionListener(mymouser);
        LeftMenuPopup.add(menuitem);
	LeftMenuPopup.setBorder(BorderFactory.createLineBorder(Color.black));

	/*
	 * And a right button popup that is active over a node.
	 */
        RightMenuPopup = new JPopupMenu("Tracker");
        menuitem = new JMenuItem("   Context Menu   ");
        RightMenuPopup.add(menuitem);
	RightMenuPopup.addSeparator();
        ShowNodeMenuItem = new JMenuItem("Emulab ShowNode");
        ShowNodeMenuItem.addActionListener(mymouser);
        RightMenuPopup.add(ShowNodeMenuItem);
        CancelDragMenuItem = new JMenuItem("Cancel Drag");
        CancelDragMenuItem.addActionListener(mymouser);
        RightMenuPopup.add(CancelDragMenuItem);
        SetOrientationMenuItem = new JMenuItem("Set Orientation");
        SetOrientationMenuItem.addActionListener(mymouser);
        RightMenuPopup.add(SetOrientationMenuItem);
        menuitem = new JMenuItem("Close This Menu");
        menuitem.addActionListener(mymouser);
        RightMenuPopup.add(menuitem);
	RightMenuPopup.setBorder(BorderFactory.createLineBorder(Color.black));

	/*
	 * Mouse listener. See below.
	 */
	addMouseListener(mymouser);
	addMouseMotionListener(mymouser);
    }

    public void start() {
        map.start();
    }
  
    public void stop() {
        map.stop();
    }

    // Simply for the benefit of running from the shell for testing.
    public void init(boolean fromshell) {
	shelled = fromshell;
	init();
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
     * A container for node and coordinate information.
     */
    private class Robot {
	String pname, vname;
	String type;			// Node type (garcia, mote, etc).
	boolean mobile;			// A robot or a fixed node (mote).
	boolean allocated;		// Node is allocated or free.
	int radius;			// Radius of circle, in pixels
	int size;			// Actual size, in pixels
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
	long last_update          = 0;	// Unix time of last update.
	String update_string      = "";
	int index;
    }
    // Indexed by the robot physical name, points Robot struct above.
    Dictionary robots     = new Hashtable();
    // Map from integer index to a Robot struct.
    Vector     robotmap   = new Vector(10, 10);
    int	       robotcount = 0;

    /*
     * A container for obstacle information. 
     */
    private class Obstacle {
	int	id;			// DB identifier; not actually used.
        int	x1, y1;			// Upper left x,y coords in pixels.
        int	x2, y2;			// Lower right x,y coords in pixels.
	String  description;
    }
    Vector	Obstacles = new Vector(10, 10);

    /*
     * A container for camera information. 
     */
    private class Camera {
	String	name;			// DB identifier; not actually used.
        int	x1, y1;			// Upper left x,y coords in pixels.
        int	x2, y2;			// Lower right x,y coords in pixels.
    }
    Vector	Cameras = new Vector(10, 10);

    private class Map extends JPanel implements Runnable {
        private Thread thread;
        private BufferedImage bimg;
	private Graphics2D G2 = null;
	private Font OurFont = null;

	// The font width/height adjustment, for drawing labels.
	int FONT_HEIGHT = 14;
	int FONT_WIDTH  = 6;

        public Map() {
	    /*
	     * Request obstacle list when a real applet.
	     */
	    if (!shelled) {
		GetNodeInfo();
		GetObstacles();
		GetCameras();
	    }

	    OurFont = new Font("Arial", Font.PLAIN, 14);
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
		G2.setFont(OurFont);
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
	    String tmp;
	    Robot robbie;
	    int index;

	    System.out.println(str);

	    tmp = tokens.nextToken().trim();	    

	    if ((robbie = (Robot) robots.get(tmp)) == null) {
		// For testing from the shell.
		if (!shelled)
		    return;
		
		robbie        = new Robot();
		index         = robotcount++;
		robbie.index  = index;
		robbie.pname  = tmp;
		robbie.radius = 18;
		robbie.size   = 27;
		robbie.type   = "garcia";
		robbie.mobile = true;
		robbie.allocated = true;
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

	    robbie.last_update   = System.currentTimeMillis();
	    now.setTime(robbie.last_update);
	    robbie.update_string = TIME_FORMAT.format(now);
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

		if ((Math.abs(robbie.y - y) < robbie.size/2 &&
		     Math.abs(robbie.x - x) < robbie.size/2) ||
		    (robbie.dragging &&
		     Math.abs(robbie.drag_y - y) < robbie.size/2 &&
		     Math.abs(robbie.drag_x - x) < robbie.size/2)) {
		    return robbie.pname;
		}
	    }
	    return "";
	}

	/*
	 * Check for overlap with exclusions zones (obstacles). We check
	 * all the robots that are dragging, and popup a dialog box if
	 * we find one. The user then has to fix it.
	 *
	 * XXX I am treating the robot as a square! Easier to calculate.
	 *
	 * XXX The value below (OBSTACLE_BUFFER) is hardwired in floormap
	 *     code (where the base image is generated).
	 */
	int OBSTACLE_BUFFER = 23;
	
	public boolean CheckforObstacles() {
	    Enumeration robot_enum = robots.elements();

	    while (robot_enum.hasMoreElements()) {
		Robot robbie  = (Robot)robot_enum.nextElement();

		if (!robbie.dragging)
		    continue;

		// Bounding box.
		int rx1 = robbie.drag_x - (robbie.radius);
		int ry1 = robbie.drag_y - (robbie.radius);
		int rx2 = robbie.drag_x + (robbie.radius);
		int ry2 = robbie.drag_y + (robbie.radius);

		//System.out.println("CheckforObstacles: " + rx1 + "," +
		//                   ry1 + "," + rx2 + "," + ry2);

		/*
		 * Check for overlap of this robot with each obstacle.
		 */
		for (int index = 0; index < Obstacles.size(); index++) {
		    Obstacle obstacle = (Obstacle) Obstacles.elementAt(index);
		    
		    int ox1 = obstacle.x1 - OBSTACLE_BUFFER;
		    int oy1 = obstacle.y1 - OBSTACLE_BUFFER;
		    int ox2 = obstacle.x2 + OBSTACLE_BUFFER;
		    int oy2 = obstacle.y2 + OBSTACLE_BUFFER;

		    //System.out.println("  " + ox1 + "," +
		    //		       oy1 + "," + ox2 + "," + cy2);

		    if (! (oy2 < ry1 ||
			   ry2 < oy1 ||
			   ox2 < rx1 ||
			   rx2 < ox1)) {
			MyDialog("Collision",
				 robbie.pname + " overlaps with an obstacle");
			return true;
		    }
		}
	    }
	    return false;
	}

	/*
	 * Check for robots wandering out of camera range. We check
	 * all the robots that are dragging, and popup a dialog box if
	 * we find one. The user then has to fix it.
	 *
	 * According to Tim, the center point of the robot needs to be
	 * within view, not the entire robot. 
	 *
	 * XXX I am treating the robot as a square! Easier to calculate.
	 */
	public boolean CheckOutOfBounds() {
	    Enumeration robot_enum = robots.elements();

	    while (robot_enum.hasMoreElements()) {
		Robot robbie  = (Robot)robot_enum.nextElement();
		int   index;

		if (!robbie.dragging)
		    continue;

		int rx1 = robbie.drag_x;
		int ry1 = robbie.drag_y;
		int rx2 = robbie.drag_x;
		int ry2 = robbie.drag_y;

		//System.out.println("CheckOutOfBounds: " + rx1 + "," +
		//                   ry1 + "," + rx2 + "," + ry2);

		/*
		 * Check for full overlap with at least one camera.
		 */
		for (index = 0; index < Cameras.size(); index++) {
		    Camera camera = (Camera) Cameras.elementAt(index);
		    
		    int cx1 = camera.x1;
		    int cy1 = camera.y1;
		    int cx2 = camera.x2;
		    int cy2 = camera.y2;

		    //System.out.println("  " + cx1 + "," +
		    //		       cy1 + "," + cx2 + "," + cy2);

		    if ((rx1 > cx1 &&
			 ry1 > cy1 &&
			 rx2 < cx2 &&
			 ry2 < cy2))
			break;
		}
		if (index == Cameras.size()) {
		    MyDialog("Out of Bounds",
			     robbie.pname + " is out of camera range");
		    return true;
		}
	    }
	    return false;
	}

	/*
	 * Check for collisions with other robots. We check all the
	 * robots that are dragging, and popup a dialog box if we find
	 * one. The user then has to fix it.
	 *
	 * XXX I am treating the robot as a square! Easier to calculate.
	 */
	public boolean CheckforCollisions() {
	    Enumeration robot_enum = robots.elements();

	    while (robot_enum.hasMoreElements()) {
		Robot robbie  = (Robot)robot_enum.nextElement();

		if (!robbie.dragging)
		    continue;

		// Bounding box.
		int rx1 = robbie.drag_x - (robbie.radius + OBSTACLE_BUFFER);
		int ry1 = robbie.drag_y - (robbie.radius + OBSTACLE_BUFFER);
		int rx2 = robbie.drag_x + (robbie.radius + OBSTACLE_BUFFER);
		int ry2 = robbie.drag_y + (robbie.radius + OBSTACLE_BUFFER);

		//System.out.println("CheckCollision: " + rx1 + "," +
		//                   ry1 + "," + rx2 + "," + ry2);

		/*
		 * Check for overlap of this robot with each other robot.
		 * If the other robot is also moving or being dragged, then
		 * check its current/target destination, not its current
		 * location. 
		 */
		for (int index = 0; index < robotmap.size(); index++) {
		    Robot mary = (Robot) robotmap.elementAt(index);
		    int   ox1, oy1, ox2, oy2;

		    if (robbie == mary)
			continue;

		    if (mary.gotdest) {
			ox1 = ox2 = mary.dx;
			oy1 = oy2 = mary.dy;
		    }
		    else if (mary.dragging) {
			ox1 = ox2 = mary.drag_x;
			oy1 = oy2 = mary.drag_y;
		    }
		    else {
			ox1 = ox2 = mary.x;
			oy1 = oy2 = mary.y;
		    }

		    ox1 -= mary.radius;
		    oy1 -= mary.radius;
		    ox2 += mary.radius;
		    oy2 += mary.radius;

		    //System.out.println("  " + ox1 + "," +
		    //		       oy1 + "," + ox2 + "," + oy2);

		    if (! (oy2 < ry1 ||
			   ry2 < oy1 ||
			   ox2 < rx1 ||
			   rx2 < ox1)) {
			MyDialog("Collision",
				 robbie.pname + " is going to run into " +
				 mary.pname);
			return true;
		    }
		}
	    }
	    return false;
	}

	/*
	 * Draw a robot, which is either the real one, a destination one,
	 * or a shadow one being dragged around.
	 */
	private int DRAWROBOT_LOC  = 1;
	private int DRAWROBOT_DST  = 2;
	private int DRAWROBOT_DRAG = 3;
	
	public void drawRobot(Graphics2D g2,
			      int x, int y, double or, String label,
			      int dot_radius, int which) {
	    /*
	     * An allocated robot is a filled circle.
	     * A destination is an unfilled circle. So is a dragging robot.
	     */
	    if (which == DRAWROBOT_LOC) {
		g2.setColor(Color.green);
		g2.fillOval(x - dot_radius, y - dot_radius,
			    dot_radius * 2, dot_radius * 2);
	    }
	    else if (which == DRAWROBOT_DST) {
		g2.setColor(Color.green);
		g2.drawOval(x - dot_radius, y - dot_radius,
			    dot_radius * 2, dot_radius * 2);
	    }
	    else if (which == DRAWROBOT_DRAG) {
		g2.setColor(Color.red);
		g2.drawOval(x - dot_radius, y - dot_radius,
			    dot_radius * 2, dot_radius * 2);
	    }
	    
	    /*
	     * If there is a orientation, add an orientation stick
	     * to it.
	     */
	    if (or != 500.0) {
		int endpoint[] = ComputeOrientationLine(x, y,
							dot_radius * 2, or);

		//System.out.println(or + " " + x + " " + y + " " +
		//                   endpoint[0] + " " + endpoint[1]);
		
		g2.drawLine(x, y, endpoint[0], endpoint[1]);
	    }

	    /*
	     * Draw a label for the robot, either above or below,
	     * depending on where the orientation stick was.
	     */
	    int lx  = x - ((FONT_WIDTH * label.length()) / 2);
	    int ly  = y + dot_radius + FONT_HEIGHT;
	    int dly = ly + FONT_HEIGHT;

	    if ((or > 180.0 && or < 360.0) ||
		(or < 0.0   && or > -180.0)) {
		ly  = y - (dot_radius + 2);
		dly = ly - FONT_HEIGHT;
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

		int dlx = x - ((FONT_WIDTH * text.length()) / 2);
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

		drawRobot(g2, x, y, robbie.or, robbie.pname,
			  (int) (robbie.size/2), DRAWROBOT_LOC);

		/*
		 * Okay, if the robot has a destination, draw that too
		 * but as a hollow circle.
		 */
		if (robbie.gotdest) {
		    int dx = robbie.dx;
		    int dy = robbie.dy;
		    
		    drawRobot(g2, dx, dy, robbie.dor, robbie.pname,
			      robbie.size/2, DRAWROBOT_DST);

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
			      robbie.size/2, DRAWROBOT_DRAG);

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
	     * table is never resized up. I am sure there is a way to
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

	    if (robbie.dragging && (col >= 6 && col <= 8))
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

	    if (robbie.dragging && (col >= 6 && col <= 8)) {
		String stmp = value.toString().trim();
		double dtmp;

		if (stmp.length() == 0)
		    return;

		try
		{
		    dtmp = Double.parseDouble(stmp);
		}
		catch(Throwable th)
		{
		    // Must not be a float.
		    return;
		}
		
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
		fireTableCellUpdated(row, col);
		repaint();
		return;
	    }
	}
    }

    /*
     * Utility function. When canceling a drag operation, we need to
     * clear the values in the table for that row. If we do not do this,
     * the next time the user tries to drag that node, the old value in
     * cell that was being edited is picked up (and I do not know how
     * to turn off editing of the cell being edited). 
     */
    private void cancelDrag(Robot robbie) {
	int row = robbie.index;
	
	robbie.dragging = false;
	maptable.clearSelection();
	// Just force the editor out of the cell, if any. This will end up
	maptable.editCellAt(100, 100);
	maptable.repaint();
    }

    /*
     * Mouse button event handler.
     */
    public class MyMouseAdaptor implements MouseInputListener, ActionListener {
	String node_id   = null;
	boolean dragging = false;
	
	public void mousePressed(MouseEvent e) {
	    int button = e.getButton();

	    if (button == e.BUTTON1) {
		/*
		 * Left button pressed. This starts a drag operation.
		 */
		if (!dragging) {
		    node_id = map.pickRobot(e.getX(), e.getY());

		    if (node_id == "") {
			node_id = null;
			MaybeLeftShowMenu(e);
			return;
		    }
		    Robot robbie = (Robot) robots.get(node_id);

		    /*
		     * Fixed nodes cannot be dragged.
		     */
		    if (! robbie.mobile) {
			node_id = null;
			return;
		    }

		    /*
		     * Do not allow robots with current destinations
		     * to be dragged. Not yet, maybe later.
		     */
		    if (robbie.gotdest) {
			node_id = null;
			return;
		    }
		    
		    robbie.drag_x   = e.getX();
		    robbie.drag_y   = e.getY();

		    /*
		     * If already dragging, then leave the dest orientation
		     * as it was. Otherwise, initialize it to the current
		     * orientation.
		     */
		    if (!robbie.dragging) {
			robbie.drag_or  = robbie.or;
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
		 * Right mouse button will bring up a context menu.
		 */
		if (! dragging) {
		    node_id = map.pickRobot(e.getX(), e.getY());

		    if (node_id == "") {
			node_id = null;
			return;
		    }
		    Robot robbie = (Robot) robots.get(node_id);
		    
		    /*
		     * Set whether the cancel move button is enabled.
		     */
		    if (robbie.dragging)
			CancelDragMenuItem.setEnabled(true);
		    else
			CancelDragMenuItem.setEnabled(false);

		    /*
		     * And set whether the orientation option is enabled.
		     */
		    if (robbie.mobile)
			SetOrientationMenuItem.setEnabled(true);
		    else
			SetOrientationMenuItem.setEnabled(false);

		    RightMenuPopup.show(map, e.getX(), e.getY());
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
			dragging = false;
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
			cancelDrag(robbie);
		    }
		    repaint();
		}
	    }
	    else if (button == e.BUTTON3) {
		/*
		 * Right mouse button brought up the context menu.
		 * It seems that this event will fire before the menu popup
		 * event fires, so nothing to do since we need to maintain
		 * the state (the selected node) for the popup event below.
		 */
		if (node_id == null) {
		    return;
		}
		System.out.println("Click finished: " + node_id);
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

		cancelDrag(robbie);
		robbie.dragging = false;
		repaint();
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

		if (Math.abs(new_x - old_x) >= 2 ||
		    Math.abs(new_y - old_y) >= 2) {
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
	public void MaybeLeftShowMenu(MouseEvent e) {
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
		    LeftMenuPopup.show(map, e.getX(), e.getY());
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
	    if (source == CancelMovesMenuItem) {
		/*
		 * Clear all the dragging bits.
		 */
		Enumeration enum = robots.elements();

		while (enum.hasMoreElements()) {
		    cancelDrag((Robot) enum.nextElement());
		}
	    }
	    else if (source == SubmitMenuItem) {
		/*
		 * Check for collisions before submitting.
		 */
		if (! map.CheckforObstacles() &&
		    ! map.CheckforCollisions() &&
		    ! map.CheckOutOfBounds())
		    SendInDestinations();
	    }
	    else if (source == ShowNodeMenuItem && node_id != null) {
		if (! shelled) {
		    Robot robbie = (Robot) robots.get(node_id);
		    
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
	    else if (source == CancelDragMenuItem && node_id != null) {
		cancelDrag((Robot) robots.get(node_id));
	    }
	    else if (source == SetOrientationMenuItem && node_id != null) {
		String saved_node_id = node_id;
		String str = (String)
		    JOptionPane.showInputDialog(map,
						"Enter new Orientation for "
						+ node_id,
						null,
						JOptionPane.PLAIN_MESSAGE);
		str = str.trim();
		System.out.println("Orientation: " + str);

		if (str.length() > 0) {
		    double dtmp;

		    try
		    {
			dtmp = Double.parseDouble(str);
		    }
		    catch(Throwable th)
		    {
			// Must not be a float.
			repaint();
			return;
		    }
		    Robot robbie = (Robot) robots.get(saved_node_id);

		    robbie.drag_or = dtmp;
		    robbie.dragor_string = str;
		    
		    /*
		     * Set things up so that the robot is now being dragged.
		     */
		    if (! robbie.dragging) {
			robbie.drag_x   = robbie.x;
			robbie.drag_y   = robbie.y;
			robbie.dragx_meters = robbie.x_meters;
			robbie.dragy_meters = robbie.y_meters;
			robbie.dragging = true;
		    }
		}
	    }
	    repaint();
	    maptable.repaint(10);
	}
    }

    /*
     * Get the node list from the server.
     */
    public boolean GetNodeInfo() {
 	String urlstring = "nodeinfo.php3?fromapplet=1"
	    + "&floor=" + floor
	    + "&building=" + building
	    + "&nocookieuid="
	    + URLEncoder.encode(uid)
	    + "&nocookieauth="
	    + URLEncoder.encode(auth);

	try
	{
	    URL			url = new URL(getCodeBase(), urlstring);
	    URLConnection	urlConn;
	    InputStream		is;
	    String		str;
	    int			index = 0;
	    
	    urlConn = url.openConnection();
	    urlConn.setDoInput(true);
	    urlConn.setUseCaches(false);
	    is = urlConn.getInputStream();
		
	    BufferedReader input
		= new BufferedReader(new InputStreamReader(is));

	    /*
	     * This should be in XML format.
	     */
	    while ((str = input.readLine()) != null) {
		System.out.println(str);

		StringTokenizer tokens = new StringTokenizer(str, ",");
		String		nodeid;
		Robot		robbie;

		nodeid = tokens.nextToken().trim();

		if ((robbie = (Robot) robots.get(nodeid)) == null) {
		    robbie       = new Robot();
		    index        = robotcount++;
		    robbie.index = index;
		    robbie.pname = nodeid;
		    robots.put(nodeid, robbie);
		    robotmap.insertElementAt(robbie, index);
		}
		robbie.vname     = tokens.nextToken().trim();
		robbie.type      = tokens.nextToken().trim();
		robbie.allocated =
		    tokens.nextToken().trim().compareTo("1") == 0;
		robbie.mobile    =
		    tokens.nextToken().trim().compareTo("1") == 0;
		robbie.size      = (int)
		    (Float.parseFloat(tokens.nextToken().trim()) *
				      pixels_per_meter);
		robbie.radius    = (int)
		    (Float.parseFloat(tokens.nextToken().trim()) *
				      pixels_per_meter);
	    }
	    is.close();
	}
	catch(Throwable th)
	{
	    MyDialog("GetNodeInfo",
		     "Failed to get node list from server");
	    th.printStackTrace();
	    return false;
	}
	return true;
    }

    /*
     * Get the Obstacle list from the server.
     */
    public boolean GetObstacles() {
 	String urlstring = "obstacles.php3?fromapplet=1"
	    + "&floor=" + floor
	    + "&building=" + building
	    + "&nocookieuid="
	    + URLEncoder.encode(uid)
	    + "&nocookieauth="
	    + URLEncoder.encode(auth);

	try
	{
	    URL			url = new URL(getCodeBase(), urlstring);
	    URLConnection	urlConn;
	    InputStream		is;
	    String		str;
	    int			index = 0;
	    
	    urlConn = url.openConnection();
	    urlConn.setDoInput(true);
	    urlConn.setUseCaches(false);
	    is = urlConn.getInputStream();
		
	    BufferedReader input
		= new BufferedReader(new InputStreamReader(is));

	    Obstacles = new Vector(10, 10);	    

	    /*
	     * This should be in XML format.
	     */
	    while ((str = input.readLine()) != null) {
		System.out.println(str);

		StringTokenizer tokens = new StringTokenizer(str, ",");
		String		tmp;
		Obstacle	obstacle = new Obstacle();

		/*
		 * Convert from meters to pixels ...
		 */
		obstacle.id = Integer.parseInt(tokens.nextToken().trim());
		obstacle.x1 = (int)
		    (Double.parseDouble(tokens.nextToken().trim())
		     * pixels_per_meter);
		obstacle.y1 = (int)
		    (Double.parseDouble(tokens.nextToken().trim())
		     * pixels_per_meter);
		obstacle.x2 = (int)
		    (Double.parseDouble(tokens.nextToken().trim())
		     * pixels_per_meter);
		obstacle.y2 = (int)
		    (Double.parseDouble(tokens.nextToken().trim())
		     * pixels_per_meter);
		obstacle.description = tokens.nextToken().trim();

		Obstacles.insertElementAt(obstacle, index++);
	    }
	    is.close();
	}
	catch(Throwable th)
	{
	    MyDialog("GetObstacles",
		     "Failed to get obstacle list from server");
	    th.printStackTrace();
	    return false;
	}
	return true;
    }

    /*
     * Get the Camera list from the server.
     */
    public boolean GetCameras() {
 	String urlstring = "cameras.php3?fromapplet=1"
	    + "&floor=" + floor
	    + "&building=" + building
	    + "&nocookieuid="
	    + URLEncoder.encode(uid)
	    + "&nocookieauth="
	    + URLEncoder.encode(auth);

	try
	{
	    URL			url = new URL(getCodeBase(), urlstring);
	    URLConnection	urlConn;
	    InputStream		is;
	    String		str;
	    int			index = 0;
	    
	    urlConn = url.openConnection();
	    urlConn.setDoInput(true);
	    urlConn.setUseCaches(false);
	    is = urlConn.getInputStream();
		
	    BufferedReader input
		= new BufferedReader(new InputStreamReader(is));

	    Cameras = new Vector(10, 10);	    

	    /*
	     * This should be in XML format.
	     */
	    while ((str = input.readLine()) != null) {
		System.out.println(str);

		StringTokenizer tokens = new StringTokenizer(str, ",");
		String		tmp;
		Camera		camera = new Camera();

		/*
		 * Convert from meters to pixels ...
		 */
		camera.name = tokens.nextToken().trim();
		camera.x1 = (int)
		    (Double.parseDouble(tokens.nextToken().trim())
		     * pixels_per_meter);
		camera.y1 = (int)
		    (Double.parseDouble(tokens.nextToken().trim())
		     * pixels_per_meter);
		camera.x2 = (int)
		    (Double.parseDouble(tokens.nextToken().trim())
		     * pixels_per_meter);
		camera.y2 = (int)
		    (Double.parseDouble(tokens.nextToken().trim())
		     * pixels_per_meter);

		Cameras.insertElementAt(camera, index++);
	    }
	    is.close();
	}
	catch(Throwable th)
	{
	    MyDialog("GetCameras",
		     "Failed to get camera list from server");
	    th.printStackTrace();
	    return false;
	}
	return true;
    }

    /*
     * Send all destinations into the web server.
     */
    public boolean SendInDestinations() {
	Enumeration e = robots.elements();

	// Its a POST, so no leading "?" for the URL arguments.
 	String urlstring = "fromapplet=1"
	    + "&nocookieuid="
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
		MyDialog("Setdest Submission Failed", str);
	    }
	}
	catch(Throwable th)
	{
	    th.printStackTrace();
	}
	return true;
    }

    /*
     * Utility function to pop up a dialog box.
     */
    public void MyDialog(String title, String msg) {
	System.out.println(title + " - " + msg);
	
	JOptionPane.showMessageDialog(getContentPane(),
				      msg, title,
				      JOptionPane.ERROR_MESSAGE);
    }

    public static void main(String argv[]) {
        final RoboTrack robomap = new RoboTrack();
	try
	{
	    URL url = new URL("file://robots-4.jpg");
	    robomap.init(true);
	    robomap.is = System.in;
	    robomap.floorimage = robomap.getImage(url);
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
