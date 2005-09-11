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
import com.sun.image.codec.jpeg.*;

/*
 * Draw the floormap and put little dots on it. 
 */
public class RoboTrack extends JApplet {
    int myHeight, myWidth;
    URL robopipeurl = null;
    Map map;
    JTable maptable;
    JPopupMenu LeftMenuPopup, RightMenuPopup;
    JMenuItem CancelMovesMenuItem, SubmitMenuItem;
    JMenuItem StopMenuItem, RestartMenuItem;
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
    WebCam WebCamWorkers[] = null;
    int WebCamCount = 0;
    
    /*
     * The connection to boss that will provide robot location info.
     * When run interactively (from main) it is set to stdin (see below).
     */
    InputStream is;

    public void init() {
	try
	{
	    if (! shelled) {
		Dimension appletSize = this.getSize();
		
		myHeight = appletSize.height;
		myWidth  = appletSize.width;
	    }
	    URL urlServer = this.getCodeBase(), floorurl, camurl;
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

	    if (this.getParameter("WebCamCount") != null) {
		int count =
		    Integer.parseInt(this.getParameter("WebCamCount").trim());
		WebCamWorkers = new WebCam[count];
		System.out.println("There are " + count + " webcams");
		
		for (int i = 0; i < count; i++) {
		    String camname   = "WebCam" + i;
		    String coordname = "WebCam" + i + "XY";
		    int x, y;

		    if (this.getParameter(camname) == null ||
			this.getParameter(coordname) == null)
			continue;

		    camurl = new URL(urlServer,
				     this.getParameter(camname)
				     + "&nocookieuid="
				     + URLEncoder.encode(uid)
				     + "&nocookieauth="
				     + URLEncoder.encode(auth));

		    StringTokenizer tokens =
			new StringTokenizer(this.getParameter(coordname), ",");

		    x = Integer.parseInt(tokens.nextToken().trim());
		    y = Integer.parseInt(tokens.nextToken().trim());

		    WebCamWorkers[WebCamCount] =
			new WebCam(WebCamCount, camurl, x, y);
		    WebCamCount++;
		}
	    }

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
	    robopipeurl = new URL(urlServer,
				  pipeurl
				  + "&nocookieuid="
				  + URLEncoder.encode(uid)
				  + "&nocookieauth="
				  + URLEncoder.encode(auth));
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

	/*
	 * Going to use the layered pane so that we can put the cameras
	 * in front of the map. But first I need to create a JPanel to
	 * contain the map and table. Why? Cause a layeredPane has no
	 * real layout management, and I do not want to use absolute
	 * positioning for that. So, put everything inside a JPanel, and
	 * then put the JPanel inside the root layeredPane. The put the
	 * webcams on top of that.
	 */
	JLayeredPane MyLPane  = getLayeredPane();
	JPanel       MyJPanel = new JPanel(true);

	/*
	 * Specify vertical placement of components inside my JPanel.
	 */
	MyJPanel.setLayout(new BoxLayout(MyJPanel, BoxLayout.Y_AXIS));

	/*
	 * Now add the basic objects to the JPanel.
	 */
	map = new Map();
        MyJPanel.add(map);

	/*
	 * This sillyness is how you get the column headers to show
	 * when you do not put the Jtable inside a scroll thingy.
	 */
	maptable = new JTable(new MyTableModel());
	MyJPanel.add(maptable.getTableHeader());
	MyJPanel.add(maptable);

	/*
	 * Add the JPanel to the layeredPane, but give its size since
	 * there is no layout manager.
	 */
	MyLPane.add(MyJPanel, JLayeredPane.DEFAULT_LAYER);
	MyJPanel.setBounds(0, 0, myWidth, myHeight);

	/*
	 * Now, if we have webcams, put them on the layered pane, on top
	 * of the the JPanel that holds the other stuff.
	 */
	if (WebCamCount > 0) {
	    for (int i = 0; i < WebCamCount; i++) {
		WebCam cam = WebCamWorkers[i];

		MyLPane.add(cam, JLayeredPane.PALETTE_LAYER);
		
		cam.setBounds(cam.X, cam.Y, cam.W, cam.H);
	    }
	}

	// This is used below for the listeners. 
	MyMouseAdaptor mymouser = new MyMouseAdaptor();
	
	/*
	 * Create the popup menu that will be used to fire off
	 * robot moves. We use the mouseadaptor for this too. 
	 */
        LeftMenuPopup = new JPopupMenu("Tracker");
        JMenuItem menuitem = new JMenuItem("    Main Menu    ");
        LeftMenuPopup.add(menuitem);
	LeftMenuPopup.addSeparator();
        SubmitMenuItem = new JMenuItem("Submit All Moves");
        SubmitMenuItem.addActionListener(mymouser);
        LeftMenuPopup.add(SubmitMenuItem);
        CancelMovesMenuItem = new JMenuItem("Cancel All Moves");
        CancelMovesMenuItem.addActionListener(mymouser);
        LeftMenuPopup.add(CancelMovesMenuItem);
        StopMenuItem = new JMenuItem("Stop Applet");
        StopMenuItem.addActionListener(mymouser);
        LeftMenuPopup.add(StopMenuItem);
        RestartMenuItem = new JMenuItem("Restart Applet");
        RestartMenuItem.addActionListener(mymouser);
        LeftMenuPopup.add(RestartMenuItem);
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

	for (int i = 0; i < WebCamCount; i++) {
	    WebCamWorkers[i].start();
	}
    }
  
    public void stop() {
	for (int i = 0; i < WebCamCount; i++) {
	    WebCamWorkers[i].stop();
	}
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
	}
	catch(Throwable th)
	{
	    th.printStackTrace();
	}
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
	boolean   dynamic   = false;	// A dynamically created Obstacle.
	Rectangle rectangle = null;	// Only for dynamic obstacles.
	Rectangle exclusion = null;	// Ditto, for the grey area.
	String    robbie    = null;     // Set to nodeid if a robot obstacle.
    }
    Vector	Obstacles = new Vector(10, 10);
    Dictionary  ObDynMap  = new Hashtable();	// Temp Obstacles only.
    int		ObCount   = 0;
    int		OBSTACLE_BUFFER = 23;	// XXX

    /*
     * A container for camera information. 
     */
    private class Camera {
	String	name;			// DB identifier; not actually used.
        int	x1, y1;			// Upper left x,y coords in pixels.
        int	x2, y2;			// Lower right x,y coords in pixels.
    }
    Vector	Cameras = new Vector(10, 10);

    // The font width/height adjustment, for drawing labels.
    int FONT_HEIGHT = 14;
    int FONT_WIDTH  = 6;
    Font OurFont    = null;

    // For exclusion zone.
    Composite ExclusionComposite;
    Stroke    ExclusionStroke;

    // A little bufferedimage to hold (cache) the scale bar.
    private BufferedImage scalebar_bimg;
    
    private class Map extends JPanel implements Runnable {
        private Thread thread, daemon;

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
	    createScale();

	    ExclusionComposite = AlphaComposite.
		getInstance(AlphaComposite.SRC_OVER, 0.15f);

	    ExclusionStroke = new BasicStroke(1.5f);
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
	 * Parse the data returned by the web server. This is bogus; we
	 * should create an XML representation of it, but thats a lot of
	 * extra overhead, and I anticipate a lot of events!
	 */
	public void parseEvent(String str) {
	    StringTokenizer	tokens;
	    String		id, type, tmp, key;
	    int			ch, delim;

	    System.out.println(str);

	    //
	    // TYPE=XXX,ID=YYY,X=1,Y=2, ...
	    //
	    tokens = new StringTokenizer(str, ",");

	    tmp   = tokens.nextToken().trim();
	    delim = tmp.indexOf('=');

	    if (delim < 0)
		return;
	    
	    key  = tmp.substring(0, delim);
	    type = tmp.substring(delim+1);
	    if (! key.equals("TYPE"))
		return;

	    tmp   = tokens.nextToken().trim();
	    delim = tmp.indexOf('=');

	    if (delim < 0)
		return;
	    
	    key  = tmp.substring(0, delim);
	    id   = tmp.substring(delim+1);
	    if (! key.equals("ID"))
		return;

	    if (type.equals("NODE")) {
		parseRobot(id, tokens);
	    }
	    else if (type.equals("AREA")) {
		parseObstacle(id, tokens);
	    }
	}

        /*
	 * Parse a robot event.
	 */
	public void parseRobot(String nodeid, StringTokenizer tokens) {
	    Robot robbie;
	    int index;

	    if ((robbie = (Robot) robots.get(nodeid)) == null) {
		// For testing from the shell.
		if (!shelled)
		    return;
		
 		robbie           = new Robot();
		index            = robotcount++;
		robbie.index     = index;
		robbie.pname     = nodeid;
		robbie.vname     = nodeid;
		robbie.radius    = 18;
		robbie.size      = 27;
		robbie.type      = "garcia";
		robbie.z         = 1.0;
		robbie.z_meters  = "1.0";
		robbie.or        = 90.0;
		robbie.or_string = "90.0";
		robbie.battery_percentage = "0.0";
		robbie.battery_voltage    = "0.0";
		robbie.mobile    = true;
		robbie.allocated = true;
		robots.put(nodeid, robbie);
		robotmap.insertElementAt(robbie, index);
	    }

	    while (tokens.hasMoreTokens()) {
		String tmp = tokens.nextToken().trim();
		int delim  = tmp.indexOf('=');

		if (delim < 0) {
		    continue;
		}

		String key = tmp.substring(0, delim);
		String val = tmp.substring(delim+1);

		if (key.equals("X")) {
		    robbie.x = Integer.parseInt(val);
		    robbie.x_meters = FORMATTER.format(robbie.x /
						       pixels_per_meter);
		}
		else if (key.equals("Y")) {
		    robbie.y = Integer.parseInt(val);
		    robbie.y_meters = FORMATTER.format(robbie.y /
						       pixels_per_meter);
		}
		else if (key.equals("OR")) {
		    robbie.or = Double.parseDouble(val);
		    robbie.or_string = FORMATTER.format(robbie.or);
		}
		else if (key.equals("DX")) {
		    if (val.equals("NULL")) {
			robbie.dx = 0;
			robbie.dx_meters  = "";
			robbie.dy = 0;
			robbie.dy_meters  = "";
			robbie.dor = 500.0;
			robbie.dor_string = "";
			robbie.gotdest = false;
		    }
		    else {
			robbie.dx  = Integer.parseInt(val);
			robbie.dx_meters  = FORMATTER.format(robbie.dx /
							     pixels_per_meter);
			robbie.gotdest = true;
		    }
		}
		else if (key.equals("DY")) {
		    if (val.equals("NULL")) {
			robbie.dy = 0;
			robbie.dy_meters  = "";
		    }
		    else {
			robbie.dy  = Integer.parseInt(val);
			robbie.dy_meters  = FORMATTER.format(robbie.dy /
							     pixels_per_meter);
		    }
		}
		else if (key.equals("DOR")) {
		    if (val.equals("NULL")) {
			robbie.dor = 500.0;
			robbie.dor_string = "";
		    }
		    else {
			robbie.dor = Double.parseDouble(val);
			robbie.dor_string = FORMATTER.format(robbie.dor);
		    }
		}
		else if (key.equals("BATV")) {
		    // Store these as strings for easy display.
		    robbie.battery_voltage = 
			FORMATTER.format(Float.parseFloat(val));

		    // This causes a robot to look alive. 
		    robbie.last_update   = System.currentTimeMillis();
		    now.setTime(robbie.last_update);
		    robbie.update_string = TIME_FORMAT.format(now);
		}
		else if (key.equals("BAT%")) {
		    // Store these as strings for easy display.
		    robbie.battery_percentage =
			FORMATTER.format(Float.parseFloat(val));
		}
	    }
	}

        /*
	 * Parse an obstacle event. We create, destroy, and modify temp
	 * obstacles so that the user notices them.
	 */
	public void parseObstacle(String obid, StringTokenizer tokens) {
	    Obstacle	oby;
	    int		index;
	    String	action = "";
	    int		x1 = 0, y1 = 0, x2 = 0, y2 = 0, id;
	    String      robotag = null;

	    id = Integer.parseInt(obid);

	    // We either know about it, or we do not ...
	    oby = (Obstacle) ObDynMap.get(obid);

	    while (tokens.hasMoreTokens()) {
		String tmp = tokens.nextToken().trim();
		int delim  = tmp.indexOf('=');

		if (delim < 0)
		    continue;

		String key = tmp.substring(0, delim);
		String val = tmp.substring(delim+1);

		if (key.equals("ACTION")) {
		    action = val;
		}
		else if (key.equals("XMIN")) {
		    x1 = Integer.parseInt(val);
		}
		else if (key.equals("XMAX")) {
		    x2 = Integer.parseInt(val);
		}
		else if (key.equals("YMIN")) {
		    y1 = Integer.parseInt(val);
		}
		else if (key.equals("YMAX")) {
		    y2 = Integer.parseInt(val);
		}
		else if (key.equals("ROBOT")) {
		    robotag = val;
		}
	    }
	    if (action.equals("CREATE")) {
		// Is this (left over ID) going to happen? Remove old one.
		// We do not want a partly initialized Obstacle in the list!
		if (oby != null) {
		    ObDynMap.remove(obid);
		}
		// Must delay insert until the new Obstacle is initialized
		oby = new Obstacle();

		oby.id = id;
		oby.x1 = x1;
		oby.y1 = y1;
		oby.x2 = x2;
		oby.y2 = y2;
		oby.description = "Dynamic Obstacle";
		oby.robbie      = robotag;
		oby.dynamic     = true;
		oby.rectangle   = new Rectangle(x1, y1, x2-x1, y2-y1);
		oby.exclusion   = new Rectangle(x1 - OBSTACLE_BUFFER,
					y1 - OBSTACLE_BUFFER,
					(x2 + OBSTACLE_BUFFER) -
						(x1 - OBSTACLE_BUFFER),
					(y2 + OBSTACLE_BUFFER) -
						(y1 - OBSTACLE_BUFFER));

		System.out.println("Creating Obstacle: " + id);
		ObDynMap.put(obid, oby);
	    }
	    else if (action.equals("CLEAR")) {
		if (oby != null) {
		    System.out.println("Clearing Obstacle: " + id);
		    
		    ObDynMap.remove(obid);
		}
	    }
	    else if (action.equals("MODIFY")) {
		// Can this happen?
		if (oby == null) {
		    return;
		}

		oby.x1 = x1;
		oby.y1 = y1;
		oby.x2 = x2;
		oby.y2 = y2;
		oby.rectangle = new Rectangle(x1, y1, x2-x1, y2-y1);
		oby.exclusion = new Rectangle(x1 - OBSTACLE_BUFFER,
					y1 - OBSTACLE_BUFFER,
					(x2 + OBSTACLE_BUFFER) -
						(x1 - OBSTACLE_BUFFER),
					(y2 + OBSTACLE_BUFFER) -
						(y1 - OBSTACLE_BUFFER));

		System.out.println("Modifying Obstacle: " + id);
	    }
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
	 * XXX The value of OBSTACLE_BUFFER is hardwired in floormap
	 *     code (where the base image is generated).
	 */
	public boolean CheckforObstacles() {
	    Enumeration robot_enum = robots.elements();

	    while (robot_enum.hasMoreElements()) {
		Robot robbie  = (Robot)robot_enum.nextElement();

		if (!robbie.dragging)
		    continue;

		/*
		 * Tim says that we only want to look to see if the
		 * center of the circle the robot sweeps, overlaps
		 * with an exclusion zone around an obstacle.
		 */
		int rx1 = robbie.drag_x;
		int ry1 = robbie.drag_y;
		int rx2 = robbie.drag_x;
		int ry2 = robbie.drag_y;

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
		    //		       oy1 + "," + ox2 + "," + oy2);

		    if (! (oy2 < ry1 ||
			   ry2 < oy1 ||
			   ox2 < rx1 ||
			   rx2 < ox1)) {
			MyDialog("Collision",
				 robbie.pname + " overlaps with an obstacle");
			return true;
		    }
		}

		/*
		 * Check Dynamic obstacles also.
		 */
		if (ObDynMap.size() > 0) {
		    Enumeration e = ObDynMap.elements();

		    while (e.hasMoreElements()) {
			Obstacle obstacle = (Obstacle)e.nextElement();

			/*
			 * Skip this Obstacle if its a robot Obstacle
			 * that was generated on the fly, and it refers
			 * to our self.
			 */
			if (obstacle.robbie != null &&
			    obstacle.robbie.equals(robbie.pname)) {
			    continue;
			}

			int ox1 = obstacle.x1 - OBSTACLE_BUFFER;
			int oy1 = obstacle.y1 - OBSTACLE_BUFFER;
			int ox2 = obstacle.x2 + OBSTACLE_BUFFER;
			int oy2 = obstacle.y2 + OBSTACLE_BUFFER;

			//System.out.println("  " + ox1 + "," +
			//		   oy1 + "," + ox2 + "," + oy2);

			if (! (oy2 < ry1 ||
			       ry2 < oy1 ||
			       ox2 < rx1 ||
			       rx2 < ox1)) {
			    MyDialog("Collision",
				     robbie.pname +
				     " overlaps with a dynamic obstacle");
			    return true;
			}
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

	    if (Cameras.size() == 0)
	        return false;

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

		    /*
		     * Check the Z axis first. If the objects are on
		     * different "planes" then no collision is possible.
		     * Okay, so this is really a hack instead, until we
		     * need something smarter.
		     */
		    if (Math.abs(robbie.z - mary.z) > 0.25)
			continue;

		    if (mary.dragging) {
			ox1 = ox2 = mary.drag_x;
			oy1 = oy2 = mary.drag_y;
		    }
		    else if (mary.gotdest) {
			ox1 = ox2 = mary.dx;
			oy1 = oy2 = mary.dy;
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
			      int dot_radius, int which, boolean stale) {
	    /*
	     * An allocated robot is a filled circle.
	     * A destination is an unfilled circle. So is a dragging robot.
	     */
	    if (which == DRAWROBOT_LOC) {
		if (stale)
		    g2.setColor(Color.blue);
		else
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
	     * If there is an orientation, add an orientation stick
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

	/*
	 * Create a scalebar. Eventually, this will be in the base image,
	 * or I can make a layered jpanel for it.
	 */
	public void createScale() {
	    String	label = "1 Meter";
	    int		dis   = (int) (pixels_per_meter == 1.0 ?
				       100 : pixels_per_meter);
	    int		sx1   = myWidth - (dis + 10);
	    int		sx2   = myWidth - 10;
	    int		dlx   = ((dis / 2) -
				 ((FONT_WIDTH * label.length()) / 2));
	    
	    scalebar_bimg = new BufferedImage(dis, 30,
					      BufferedImage.TYPE_INT_ARGB);
	    Graphics2D G2 = scalebar_bimg.createGraphics();
	    G2.setFont(OurFont);

	    G2.setRenderingHint(RenderingHints.KEY_ANTIALIASING,
				RenderingHints.VALUE_ANTIALIAS_ON);
	    G2.setBackground(Color.white);
	    G2.setColor(Color.black);
	    G2.drawLine(0, 10, dis - 1, 10);
	    G2.drawLine(0, 5,  0, 15);
	    G2.drawLine(dis - 1, 5,  dis - 1, 15);
	    G2.drawString(label, dlx, 30);
	}

	public void drawMap(int w, int h, Graphics2D g2) {
	    int fw, fh;
	    
	    fw = floorimage.getWidth(this);
	    fh = floorimage.getHeight(this);

	    /*
	     * The base image is the floormap and the scalebar.
	     */
	    g2.drawImage(floorimage, 0, 0, fw, fh, this);
	    g2.drawImage(scalebar_bimg, myWidth - 125, 0, this);

	    g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING,
				RenderingHints.VALUE_ANTIALIAS_ON);

	    /*
	     * Dyamnic obstacles first
	     */
	    if (ObDynMap.size() > 0) {
		Enumeration e = ObDynMap.elements();
		Composite   savedcomposite = g2.getComposite();
		Stroke      savedstroke    = g2.getStroke();

		g2.setPaint(Color.black);
		g2.setComposite(ExclusionComposite);
		g2.setStroke(ExclusionStroke);
		g2.setColor(Color.blue);

		while (e.hasMoreElements()) {
		    Obstacle  oby  = (Obstacle)e.nextElement();
		    Rectangle rectangle = oby.rectangle;
		    Rectangle exclusion = oby.exclusion;

		    if (rectangle != null) {
			g2.draw(rectangle);
		    }
		    if (exclusion != null) {
			g2.fill(exclusion);
		    }
		}
		g2.setComposite(savedcomposite);
		g2.setStroke(savedstroke);
	    }
	    
	    /*
	     * Then we draw a bunch of stuff on it, like the robots.
	     */
	    Enumeration e = robots.elements();

	    while (e.hasMoreElements()) {
		Robot robbie  = (Robot)e.nextElement();

		int x = robbie.x;
		int y = robbie.y;
		boolean stale = (robbie.last_update == 0);

		drawRobot(g2, x, y, robbie.or, robbie.pname,
			  (int) (robbie.size/2), DRAWROBOT_LOC, stale);

		/*
		 * Okay, if the robot has a destination, draw that too
		 * but as a hollow circle.
		 */
		if (robbie.gotdest) {
		    int dx = robbie.dx;
		    int dy = robbie.dy;
		    
		    drawRobot(g2, dx, dy, robbie.dor, robbie.pname,
			      robbie.size/2, DRAWROBOT_DST, false);

		    /*
		     * And draw a light grey line from source to dest.
		     */
		    g2.setColor(Color.gray);
		    g2.drawLine(x, y, dx, dy);
		}
		if (robbie.dragging) {
		    int dx = robbie.drag_x;
		    int dy = robbie.drag_y;
		    
		    drawRobot(g2, dx, dy, robbie.drag_or, robbie.pname,
			      robbie.size/2, DRAWROBOT_DRAG, false);

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

	    Graphics2D g2 = (Graphics2D)g;
		
	    g2.setFont(OurFont);
	    g2.clearRect(0, 0, w, h);
	    drawMap(w, h, g2);
        }
    
        public void start() {
            daemon = new Thread(this);
            daemon.start();
            thread = new Thread(this);
            thread.start();
        }
    
        public synchronized void stop() {
            thread = null;
            daemon = null;
        }

	public void Daemon() {
            Thread me = Thread.currentThread();
	    
	    System.out.println("Daemon Starting");
	    while (me == daemon) {
                try {
                    me.sleep(1000);
                }
		catch (InterruptedException e)
		{
		    break;
		}
		long now = System.currentTimeMillis();
		Enumeration e = robots.elements();

		while (e.hasMoreElements()) {
		    Robot robbie  = (Robot)e.nextElement();

		    if (robbie.last_update == 0)
			continue;

		    if (now - robbie.last_update >= (5 * 60 * 1000)) {
			System.out.println(robbie.pname + " has gone stale");
			robbie.last_update = 0;
		    }
		}
		repaint();
		maptable.repaint(10);
	    }
	    System.out.println("Daemon Stopping");
	}
    
        public void run() {
            Thread me = Thread.currentThread();
	    String str;

	    // See if we are the daemon thread.
	    if (me == daemon) {
		Daemon();
		return;
	    }

	    if (! shelled) {
		System.out.println("Opening URL: " + robopipeurl.toString());
		
		try
	        {
		    URLConnection uc = robopipeurl.openConnection();
		    uc.setDoInput(true);
		    uc.setUseCaches(false);
		    is = uc.getInputStream();
		}
		catch(Throwable th)
	        {
		    th.printStackTrace();
		    daemon = null;
		    thread = null;
		    MyDialog("RoboPipe",
			     "Could not connect to pipe; " +
			     "is there an experiment running?");
		    return;
		}
	    }
	    BufferedReader input
		= new BufferedReader(new InputStreamReader(is));
	    
            while (thread == me) {
		try
		{
		    while (null != ((str = input.readLine()))) {
			parseEvent(str);
			repaint();
			maptable.repaint(10);
			if (thread == null)
			    break;
		    }
		    if (str == null)
		        break;
		}
		catch(IOException e)
		{
		    e.printStackTrace();
		    break;
		}
                try {
                    me.sleep(1000);
                }
		catch (InterruptedException e)
		{
		    break;
		}
            }
	    System.out.println("Returning from main thread");
            thread = null;
	    destroy();
        }

	/*
	 * Catch termination.
	 */
	public void destroy() {
	    try
	    {
	        if (!shelled && is != null)
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

	    if (e.isPopupTrigger()) {
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
	    else if (button == e.BUTTON1) {
		/*
		 * Left button pressed. This starts a drag operation.
		 */
		if (!dragging) {
		    node_id = map.pickRobot(e.getX(), e.getY());

		    if (node_id == "") {
			node_id = null;
			ShowLeftMenu(e);
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
	}
	
	public void mouseReleased(MouseEvent e) {
	    int button = e.getButton();

	    if (e.isPopupTrigger()) {
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
	    else if (button == e.BUTTON1) {
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
	 * Show left popup.
	 */
	public void ShowLeftMenu(MouseEvent e) {
	    boolean     dragging = false;
	    Enumeration enum     = robots.elements();

	    /*
	     * See if any nodes being dragged. Activate/Inactivate options
	     */
	    while (enum.hasMoreElements()) {
		Robot robbie  = (Robot)enum.nextElement();

		if (robbie.dragging) {
		    dragging = true;
		    break;
		}
	    }
	    if (dragging) {
		SubmitMenuItem.setEnabled(true);
		CancelMovesMenuItem.setEnabled(true);
	    }
	    else {
		SubmitMenuItem.setEnabled(false);
		CancelMovesMenuItem.setEnabled(false);
	    }
	    
	    /*
	     * Use "map" (us) as the invoking component so that the 
	     * menu is fully contained within the applet; this avoids
	     * the "Java Applet Window" warning that tells people
	     * a window has just been popped up.
	     */
	    LeftMenuPopup.show(map, e.getX(), e.getY());
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
	    if (source == StopMenuItem) {
		if (!frozen) {
		    frozen = true;
		    StopMenuItem.setEnabled(false);		    
		    System.out.println("Stopping applet ...");
		    stop();
		}
	    }
	    else if (source == RestartMenuItem) {
		if (!frozen) {
		    frozen = true;
		    StopMenuItem.setEnabled(false);		    
		    System.out.println("Stopping applet ...");
		    stop();
		}
		frozen = false;
		StopMenuItem.setEnabled(true);
		System.out.println("Restarting applet ...");
		start();
	    }
	    else if (source == CancelMovesMenuItem) {
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
		String		nodeid, tmp;
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
		// Skip dead
		tokens.nextToken();
		robbie.mobile    =
		    tokens.nextToken().trim().compareTo("1") == 0;
		robbie.size      = (int)
		    (Float.parseFloat(tokens.nextToken().trim()) *
				      pixels_per_meter);
		robbie.radius    = (int)
		    (Float.parseFloat(tokens.nextToken().trim()) *
				      pixels_per_meter);
		
		robbie.x     = Integer.parseInt(tokens.nextToken().trim());
		robbie.y     = Integer.parseInt(tokens.nextToken().trim());
		robbie.x_meters = FORMATTER.format(robbie.x /
						   pixels_per_meter);
		robbie.y_meters = FORMATTER.format(robbie.y /
						   pixels_per_meter);

		tmp = tokens.nextToken().trim();
		if (tmp.length() > 0) {
		    robbie.z = Double.parseDouble(tmp);
		    robbie.z_meters = FORMATTER.format(robbie.z);
		}
		else {
		    robbie.z        = 0.0;
		    robbie.z_meters = "";
		}

		tmp = tokens.nextToken().trim();
		if (tmp.length() > 0) {
		    robbie.or = Double.parseDouble(tmp);
		    robbie.or_string = FORMATTER.format(robbie.or);
		}
		else {
		    robbie.or        = 500.0;
		    robbie.or_string = "";
		}
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

		Obstacles.insertElementAt(obstacle, ObCount++);
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

    public class WebCam extends JPanel implements Runnable {
	private BufferedImage	curimage = null;
	private URL		myurl;
        private Thread		camthread;
	private int		myid = 0;
	private InputStream	is = null;
	public  int		X, Y, W, H;
	
        public WebCam(int id, URL webcamurl, int x, int y) {
	    myid  = id;
	    myurl = webcamurl;
	    X     = x;
	    Y     = y;
	    W     = 240;
	    H     = 180;

	    System.out.println("Creating a WebCam class: " +
			       id + "," + X + "," + Y);

	    super.setDoubleBuffered(true);
        }

        public void start() {
            camthread = new Thread(this);
            camthread.start();
        }
    
        public synchronized void stop() {
            camthread = null;
        }
    
	/*
	 * This is the callback to paint the map. 
	 */
        public void paint(Graphics g) {
            Dimension dim  = getSize();
	    int h = dim.height;
	    int w = dim.width;

	    if (curimage == null)
		return;
	    
	    g.drawImage(curimage, 0, 0, this);
        }

        public void run() {
            Thread me = Thread.currentThread();
	    String lenhdr = "content-length: ";
	    byte imageBytes[] = new byte[100000];
	    ByteArrayInputStream imageinput;
	    String str;
	    int size;

	    System.out.println("Opening URL: " + myurl.toString());

	    try
	    {
		// And connect to it.
		URLConnection uc = myurl.openConnection();
		uc.setDoInput(true);
		uc.setUseCaches(false);
		this.is = uc.getInputStream();
	    }
	    catch(Throwable th)
	    {
	        th.printStackTrace();
		camthread = null;
		return;
	    }
	    BufferedInputStream input = new BufferedInputStream(this.is);

            while (camthread == me) {
		try
		{
		    // First line is the boundry marker
		    if ((str = readLine(input).trim()) == null)
			break;

		    if (!str.equals("--myboundary")) {
			System.out.println("Sync error at boundry");
			break;
		    }

		    // Next line is the Content-type. Skip it.
		    if ((str = readLine(input)) == null)
			break;

		    // Next line is the Content-length. We need this. 
		    if ((str = readLine(input)) == null)
			break;
		    str = str.toLowerCase();

		    if (! str.startsWith(lenhdr)) {
			System.out.println("Sync error at content length");
			break;
		    }
		    str  = str.substring(lenhdr.length(), str.length());
		    size = Integer.parseInt(str.trim());
		    
		    if (size < 0) {
			System.out.println("Bad content length: " + size);
			break;
		    }
		    //System.out.println("content length: " + size);
		    
		    // Next line is a blank line. Skip it.
		    if ((str = readLine(input)) == null)
			break;

		    // Now we have the data. Buffer that up.
		    int count = 0;
		    while (count < size) {
			int cc = input.read(imageBytes, count, size - count);
			if (cc == -1)
			    break;

			count += cc;
		    }
		    if (count != size) {
			System.out.println("Not enough image data");
			break;
		    }
		    // For the jpeg decoder.
		    imageinput = new ByteArrayInputStream(imageBytes);
		    
		    JPEGImageDecoder decoder =
			JPEGCodec.createJPEGDecoder(imageinput);
		    
		    curimage = decoder.decodeAsBufferedImage();

		    //System.out.println("Got the image");
		    
		    // Next line is a blank line. Skip it.
		    if ((str = readLine(input)) == null)
			break;
		    
		    repaint();
		    me.yield();
		}
		catch(IOException e)
		{
		    e.printStackTrace();
		    break;
		}
            }
            camthread = null;
	    destroy();
        }

	/*
	 * Catch termination.
	 */
	public void destroy() {
	    try
	    {
	        if (is != null) {
		    is.close();
		    is = null;
		}
	    }
	    catch(IOException e)
	    {
		e.printStackTrace();
	    }
	}

	/*
	 * A utility function since a BufferedInputStream has no readline().
	 */
	private byte[] buf = new byte[1024];
	
	private String readLine(BufferedInputStream input) {
	    int		index = 0;
	    int		ch;

	    try
	    {
		while ((ch = input.read()) != -1) {
		    buf[index++] = (byte) ch;
		    if (ch == '\n')
			break;
		}
	    }
	    catch(IOException e)
	    {
		e.printStackTrace();
	    }
	    if (index == 0)
		return "";
	    return new String(buf, 0, index);
	}
    }
    
    public static void main(String argv[]) {
	int W = 900;
	int H = 600;
	
        final RoboTrack robomap = new RoboTrack();
	try
	{
	    URL url = new URL("file:///tmp/robots-4.jpg");
	    robomap.floorimage = robomap.getImage(url);
	    robomap.myWidth = W;
	    robomap.myHeight = H;
	    robomap.init(true);
	    robomap.is = System.in;
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
        f.setSize(new Dimension(W,H));
        f.show();
        robomap.start();
    }
}
