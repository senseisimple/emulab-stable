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
public class NodeSelect extends JApplet {
    Map map;
    JPopupMenu LeftMenuPopup, RightMenuPopup, NodeMenuPopup;
    JMenuItem CancelAllMenuItem, SubmitMenuItem, ShowExpMenuItem;
    JMenuItem CancelAssignMenuItem, ShowNodeMenuItem, AssignMenuItem;
    Dictionary MenuItems = new Hashtable();
    Image floorimage;
    double pixels_per_meter = 1.0;
    static final DecimalFormat FORMATTER = new DecimalFormat("0.00");
    static final SimpleDateFormat TIME_FORMAT =
	new SimpleDateFormat("hh:mm:ss");
    static final Date now = new Date();
    String uid, auth, building, floor, pid, eid;
    boolean shelled = false;
    MyMouseAdaptor mymouser;
    
    public void init() {
	try
	{
	    URL urlServer = this.getCodeBase(), floorurl;
	    String pipeurl, baseurl;
	    URLConnection uc;

	    /* Get our parameters then */
	    uid      = this.getParameter("uid");
	    auth     = this.getParameter("auth");
	    baseurl  = this.getParameter("floorurl");
	    building = this.getParameter("building");
	    floor    = this.getParameter("floor");
	    pid      = this.getParameter("pid");
	    eid      = this.getParameter("eid");
	    pixels_per_meter = Double.parseDouble(this.getParameter("ppm"));

	    // form the URL that we use to get the background image
	    floorurl = new URL(urlServer,
			       baseurl
			       + "&nocookieuid="
			       + URLEncoder.encode(uid)
			       + "&nocookieauth="
			       + URLEncoder.encode(auth));

	    System.out.println(urlServer.toString());
	    System.out.println(floorurl.toString());

	    // and then get it.
	    floorimage = getImage(floorurl);
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
	mymouser = new MyMouseAdaptor();
	
	/*
	 * Create the popup menu that will be active over the background.
	 */
        LeftMenuPopup = new JPopupMenu("Tracker");
        JMenuItem menuitem = new JMenuItem("   Main Menu   ");
        LeftMenuPopup.add(menuitem);
	LeftMenuPopup.addSeparator();
        ShowExpMenuItem = new JMenuItem("Show Experiment");
        ShowExpMenuItem.addActionListener(mymouser);
        LeftMenuPopup.add(ShowExpMenuItem);
        SubmitMenuItem = new JMenuItem("Submit Selections");
        SubmitMenuItem.addActionListener(mymouser);
        LeftMenuPopup.add(SubmitMenuItem);
        CancelAllMenuItem = new JMenuItem("Cancel All Selections");
        CancelAllMenuItem.addActionListener(mymouser);
        LeftMenuPopup.add(CancelAllMenuItem);
        menuitem = new JMenuItem("Close This Menu");
        menuitem.addActionListener(mymouser);
        LeftMenuPopup.add(menuitem);
	LeftMenuPopup.setBorder(BorderFactory.createLineBorder(Color.black));

	/*
	 * And a right button popup that is active over a node.
	 */
        RightMenuPopup = new JPopupMenu("Selector");
        menuitem = new JMenuItem("   Context Menu   ");
        RightMenuPopup.add(menuitem);
	RightMenuPopup.addSeparator();
        ShowNodeMenuItem = new JMenuItem("Emulab ShowNode");
        ShowNodeMenuItem.addActionListener(mymouser);
        RightMenuPopup.add(ShowNodeMenuItem);
        CancelAssignMenuItem = new JMenuItem("Cancel Assignment");
        CancelAssignMenuItem.addActionListener(mymouser);
        RightMenuPopup.add(CancelAssignMenuItem);
        menuitem = new JMenuItem("Close This Menu");
        menuitem.addActionListener(mymouser);
        RightMenuPopup.add(menuitem);
	RightMenuPopup.setBorder(BorderFactory.createLineBorder(Color.black));


	/*
	 * And a right button popup that is active over a node.
	 */
        NodeMenuPopup = new JPopupMenu("Selector");
        menuitem = new JMenuItem("   Node Menu   ");
        NodeMenuPopup.add(menuitem);
	NodeMenuPopup.addSeparator();
        menuitem = new JMenuItem("Close This Menu");
        menuitem.addActionListener(mymouser);
        NodeMenuPopup.add(menuitem);
	NodeMenuPopup.setBorder(BorderFactory.createLineBorder(Color.black));

	/*
	 * Mouse listener. See below.
	 */
	addMouseListener(mymouser);
	addMouseMotionListener(mymouser);

	/*
	 * Vertical placement of Components in the pane. 
	 */
	getContentPane().setLayout(new BoxLayout(getContentPane(),
						 BoxLayout.PAGE_AXIS));
        getContentPane().add(map = new Map());
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
	}
	catch (Throwable th)
	{
	    th.printStackTrace();
	}
	return img;
    }

    /*
     * A container for node and coordinate information.
     */
    private class Node {
	String pname, vname;
	String type;			// Node type (garcia, mote, etc).
	boolean mobile;			// A robot or a fixed node (mote).
	boolean allocated;		// Node is allocated or free.
	int radius;			// Radius of circle, in pixels
	int size;			// Actual size, in pixels
        int x, y;			// Current x,y coords in pixels
	double z;			// Current z in meters
	double or = 500.0;		// Current orientation 
	
	/*
	 * These are formatted as strings to avoid doing conversons
	 * on the fly when the table is redrawn. Note, we have to
	 * convert from pixel coords (what the server sends) to meters.
	 */
	String x_meters           = "";
	String y_meters           = "";
	String z_meters           = "";
	String or_string          = "";
	int index;
    }
    // Indexed by the robot physical name, points Robot struct above.
    Dictionary Nodes     = new Hashtable();
    // Map from integer index to a Robot struct.
    Vector     nodemap   = new Vector(10, 10);
    int	       nodecount = 0;

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
	    if (shelled) {
		for (int i = 0; i < 3; i++) {
		    Node	robbie;
		    int		index;
		    
		    robbie       = new Node();
		    index        = nodecount++;
		    robbie.index = index;
		    robbie.pname = "mote" + i;
		    Nodes.put(robbie.pname, robbie);
		    nodemap.insertElementAt(robbie, index);

		    robbie.type      = "emote";
		    robbie.allocated = false;
		    robbie.mobile    = false;
		    robbie.size      = 10;
		    robbie.radius    = 10;
		    robbie.x         = 100 + (100 * index);
		    robbie.y         = 100 + (100 * index);
		    robbie.x_meters  = 
			FORMATTER.format(robbie.x / pixels_per_meter);
		    robbie.y_meters  =
			FORMATTER.format(robbie.y / pixels_per_meter);

		    robbie.z = 2.5;
		    robbie.z_meters = FORMATTER.format(robbie.z);

		    robbie.or        = 500.0;
		    robbie.or_string = "";
		}
		JMenuItem menuitem = new JMenuItem("nodea");
		menuitem.addActionListener(mymouser);
		NodeMenuPopup.add(menuitem);
		MenuItems.put("nodea", menuitem);
		
		menuitem = new JMenuItem("nodeb");
		menuitem.addActionListener(mymouser);
		NodeMenuPopup.add(menuitem);
		MenuItems.put("nodeb", menuitem);
		
		menuitem = new JMenuItem("nodec");
		menuitem.addActionListener(mymouser);
		NodeMenuPopup.add(menuitem);
		MenuItems.put("nodec", menuitem);
	    }
	    else {
		GetNodeInfo();
		GetExpInfo();
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
	 * Given an x,y from a button click, try to map the coords
	 * to a robot that has been drawn on the screen. There are
	 * probably race conditions here, but I doubt they will cause
	 * any real problems (like, maybe we pick the wrong robot).
	 */
	public String pickRobot(int x, int y) {
	    Enumeration e = Nodes.elements();

	    while (e.hasMoreElements()) {
		Node robbie  = (Node)e.nextElement();

		if ((Math.abs(robbie.y - y) < robbie.size/2 &&
		     Math.abs(robbie.x - x) < robbie.size/2)) {
		    return robbie.pname;
		}
	    }
	    return "";
	}

	/*
	 * Draw a node.
	 */
	public void drawNode(Graphics2D g2,
			      int x, int y, double or, String label,
			      int dot_radius, boolean dead, boolean assigned) {
	    /*
	     * An allocated robot is a filled circle.
	     * A destination is an unfilled circle. So is a dragging robot.
	     */
	    if (dead) {
		g2.setColor(Color.red);
		g2.fillOval(x - dot_radius, y - dot_radius,
			    dot_radius * 2, dot_radius * 2);
	    }
	    else if (assigned) {
		g2.setColor(Color.yellow);
		g2.fillOval(x - dot_radius, y - dot_radius,
			    dot_radius * 2, dot_radius * 2);
	    }
	    else {
		g2.setColor(Color.green);
		g2.fillOval(x - dot_radius, y - dot_radius,
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
	    Enumeration e = Nodes.elements();

	    while (e.hasMoreElements()) {
		Node robbie  = (Node)e.nextElement();

		int x = robbie.x;
		int y = robbie.y;
		String label = robbie.pname;

		if (robbie.vname != null)
		    label += "(" + robbie.vname + ")";

		drawNode(g2, x, y, robbie.or, label,
			 (int) (robbie.size/2), false,
			 (robbie.vname == null ? false : true));
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
	    
            while (thread == me) {
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
    }
    
    /*
     * Utility function. When canceling a drag operation, we need to
     * clear the values in the table for that row. If we do not do this,
     * the next time the user tries to drag that node, the old value in
     * cell that was being edited is picked up (and I do not know how
     * to turn off editing of the cell being edited). 
     */
    private void cancelSelection(Node robbie) {
	if (robbie.vname != null) {
	    JMenuItem menuitem = (JMenuItem) MenuItems.get(robbie.vname);
	    menuitem.setEnabled(true);
	    robbie.vname = null;
	}
    }

    /*
     * Mouse button event handler.
     */
    public class MyMouseAdaptor implements MouseInputListener, ActionListener {
	String node_id   = null;
	
	public void mousePressed(MouseEvent e) {
	    int button = e.getButton();

	    if (button == e.BUTTON1) {
		/*
		 * Left button pressed. This starts a select operation.
		 */
		node_id = map.pickRobot(e.getX(), e.getY());

		if (node_id == "") {
		    node_id = null;
		    MaybeLeftShowMenu(e);
		    return;
		}
		Node robbie = (Node) Nodes.get(node_id);

		/*
		 * Not allowed to reassign a node. For simplicity, must
		 * first unassign.
		 */
		if (robbie.vname != null) {
		    node_id = null;
		    return;
		}
		NodeMenuPopup.show(map, e.getX(), e.getY());
	    }
	    else if (button == e.BUTTON3) {
		/*
		 * Right mouse button will bring up a context menu.
		 */
		node_id = map.pickRobot(e.getX(), e.getY());

		if (node_id == "") {
		    node_id = null;
		    return;
		}
		Node robbie = (Node) Nodes.get(node_id);

		/*
		 * Set whether the cancel move button is enabled.
		 */
		if (robbie.vname == null)
		    CancelAssignMenuItem.setEnabled(false);
		else
		    CancelAssignMenuItem.setEnabled(true);

		RightMenuPopup.show(map, e.getX(), e.getY());
	    }
	}
	
	public void mouseReleased(MouseEvent e) {
	    int button = e.getButton();

	    if (button == e.BUTTON1) {
		/*
		 * Left button released. This terminates the operation.
		 */
		// Make sure we received the down event.
		if (node_id == null) {
		    return;
		}
		System.out.println("Select finished: " + node_id);
		repaint();
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
		repaint();
	    }
	}

	public void mouseEntered(MouseEvent e) {
	}

	public void mouseExited(MouseEvent e) {
	}

	public void mouseClicked(MouseEvent e) {
	}

	public void mouseDragged(MouseEvent e) {
	}

	public void mouseMoved(MouseEvent e) {
	}

	/*
	 * Decide if we want to actually show the popupmenu.
	 */
	public void MaybeLeftShowMenu(MouseEvent e) {
	    LeftMenuPopup.show(map, e.getX(), e.getY());
	}

	/*
	 * This is for the popup menu(s).
	 */
	public void actionPerformed(ActionEvent e) {
	    JMenuItem source = (JMenuItem)(e.getSource());
	    String    choice = source.getText();
	    
	    System.out.println("Popup select: " + choice + "\n");

	    /*
	     * Figuring out which item was selected is silly.
	     */
	    if (source == CancelAllMenuItem) {
		/*
		 * Clear all the dragging bits.
		 */
		Enumeration enum = Nodes.elements();

		while (enum.hasMoreElements()) {
		    cancelSelection((Node) enum.nextElement());
		}
	    }
	    else if (source == SubmitMenuItem) {
		/*
		 * Check for collisions before submitting.
		 */
		SendInSelections();
	    }
	    else if (source == ShowNodeMenuItem && node_id != null) {
		if (! shelled) {
		    Node robbie = (Node) Nodes.get(node_id);
		    
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
	    else if (source == ShowExpMenuItem) {
		if (! shelled) {
		    // This will fail when I run it from the shell
		    try
		    {
			URL url = new URL(getCodeBase(),
					  "/showexp.php3?pid=" + pid
					  + "&eid=" + eid
					  + "&nocookieuid="
					  + URLEncoder.encode(uid)
					  + "&nocookieauth="
					  + URLEncoder.encode(auth));
			System.out.println(url.toString());
			getAppletContext().showDocument(url, "_blank");
		    }
		    catch(Throwable th)
		    {
			th.printStackTrace();
		    }
		}
	    }
	    else if (source == CancelAssignMenuItem && node_id != null) {
		cancelSelection((Node) Nodes.get(node_id));
	    }
	    else if (node_id != null) {
		JMenuItem menuitem = (JMenuItem) MenuItems.get(choice);

		if (source == menuitem) {
		    Node robbie = (Node) Nodes.get(node_id);

		    robbie.vname = choice;
		    menuitem.setEnabled(false);
		}
	    }
	    repaint();
	}
    }

    /*
     * Get the physical node list from the server.
     */
    public boolean GetNodeInfo() {
 	String urlstring = "nodeinfo.php3?fromapplet=1&selector=1"
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
		Node		robbie;

		nodeid = tokens.nextToken().trim();

		if ((robbie = (Node) Nodes.get(nodeid)) == null) {
		    robbie       = new Node();
		    index        = nodecount++;
		    robbie.index = index;
		    robbie.pname = nodeid;
		    Nodes.put(nodeid, robbie);
		    nodemap.insertElementAt(robbie, index);
		}
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
		
		robbie.x         = Integer.parseInt(tokens.nextToken().trim());
		robbie.y         = Integer.parseInt(tokens.nextToken().trim());
		robbie.x_meters  =
		    FORMATTER.format(robbie.x / pixels_per_meter);
		robbie.y_meters  =
		    FORMATTER.format(robbie.y / pixels_per_meter);

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
     * Get experiment info from the server. Just vnodes for now
     */
    public boolean GetExpInfo() {
 	String urlstring = "virtinfo.php3?fromapplet=1"
	    + "&pid=" + pid
	    + "&eid=" + eid
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
		String		vname, pname;
		
		vname = tokens.nextToken().trim();
		pname = tokens.nextToken().trim();

		/*
		 * Create a menu item for it.
		 */
		JMenuItem menuitem = new JMenuItem(vname);
		menuitem.addActionListener(mymouser);
		NodeMenuPopup.add(menuitem);

		MenuItems.put(vname, menuitem);

		/*
		 * Check for assignment.
		 */
		if (pname.length() > 0) {
		    Node robbie = (Node) Nodes.get(pname);

		    robbie.vname = vname;
		    menuitem.setEnabled(false);
		}
	    }
	    is.close();
	}
	catch(Throwable th)
	{
	    MyDialog("GetExpInfo",
		     "Failed to get virtual node list from server");
	    th.printStackTrace();
	    return false;
	}
	return true;
    }

    /*
     * Send the selections into the web server.
     */
    public boolean SendInSelections() {
	Enumeration e = Nodes.elements();

	// Its a POST, so no leading "?" for the URL arguments.
 	String urlstring = "fromapplet=1"
	    + "&pid=" + pid
	    + "&eid=" + eid
	    + "&nocookieuid="
	    + URLEncoder.encode(uid)
	    + "&nocookieauth="
	    + URLEncoder.encode(auth);

	while (e.hasMoreElements()) {
	    Node robbie  = (Node)e.nextElement();

	    if (robbie.vname == null) {
		continue;
	    }

	    urlstring = urlstring +
		"&nodelist[" + robbie.vname + "]=" + robbie.pname;
	}
	if (shelled) {
	    System.out.println(urlstring);
	    return true;
	}

	// This will fail when I run it from the shell
	try
	{
	    URL			url = new URL(getCodeBase(),
					      "assignnodes.php3");
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
		MyDialog("Assignment Failed", str);
	    }
	    else {
		MyDialog("Assignment Successful",
			 "Left click and select Show Experiment");
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
        final NodeSelect robomap = new NodeSelect();
	try
	{
	    URL url = new URL("file://robots-4.jpg");
	    robomap.init(true);
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
