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
import ScrollablePicture.*;

/*
 * Draw the floormap and put little dots on it. 
 */
public class NodeSelect extends JApplet {
    private boolean		shelled = false;
    private Selector		selector;
    int				myHeight, myWidth;
    String			uid, auth, pid, eid, floor, building;
    double			pixels_per_meter = 1.0;
    URL				urlServer;
    static final DecimalFormat  FORMATTER = new DecimalFormat("0.00");
    
    /* Constants for virt image link */
    int ZOOM   = 3;
    int DETAIL = 2;

    int X_ADJUST = 60;
    int Y_ADJUST = 60;

    public void init() {
	URL		expURL = null;
	URL		phyURL = null;
	String		phyurl;

	try
	{
	    /* Get our parameters */
	    uid      = this.getParameter("uid");
	    auth     = this.getParameter("auth");
	    pid      = this.getParameter("pid");
	    eid      = this.getParameter("eid");
	    floor    = this.getParameter("floor");
	    building = this.getParameter("building");
	    phyurl   = this.getParameter("floorurl");
	    pixels_per_meter = Double.parseDouble(this.getParameter("ppm"));
	    
	    // form the URL that we use to get the virtual experiment icon
	    expURL = new URL(this.getCodeBase(),
			     "../top2image.php3"
			     + "?zoom=" + ZOOM
			     + "&detail=" + DETAIL
			     + "&pid=" + pid
			     + "&eid=" + eid
			     + "&nocookieuid="
			     + URLEncoder.encode(uid)
			     + "&nocookieauth="
			     + URLEncoder.encode(auth));

	    // form the URL that we use to get the physical layout icon
	    phyURL = new URL(this.getCodeBase(),
			     phyurl
	 		     + "&pid=" + pid
			     + "&eid=" + eid
			     + "&nocookieuid="
			     + URLEncoder.encode(uid)
			     + "&nocookieauth="
			     + URLEncoder.encode(auth));
	}
	catch(Throwable th)
	{
	    th.printStackTrace();
	}
	init(false, expURL, phyURL);
    }

    public void start() {
    }
  
    public void stop() {
    }

    // For the benefit of running from the shell for testing.
    public void init(boolean fromshell, URL expURL, URL phyURL) {
	shelled = fromshell;

	if (!shelled) {
	    Dimension appletSize = this.getSize();
		
	    myHeight = appletSize.height;
	    myWidth  = appletSize.width;
	}

	JLayeredPane MyLPane  = getLayeredPane();
	JPanel       MyJPanel = new JPanel(true);

	/*
	 * Specify vertical placement of components inside my JPanel.
	 */
	MyJPanel.setLayout(new BoxLayout(MyJPanel, BoxLayout.Y_AXIS));

	/*
	 * Now add the basic objects to the JPanel.
	 */
	selector = new Selector(expURL, phyURL);
        MyJPanel.add(selector);

	/*
	 * Add the JPanel to the layeredPane, but give its size since
	 * there is no layout manager.
	 */
	MyLPane.add(MyJPanel, JLayeredPane.DEFAULT_LAYER);
	MyJPanel.setBounds(0, 0, myWidth, myHeight);
    }

    /*
     * A container for virtual node info.
     */
    private class VirtNode {
	String		vname;
	String		pname;
	String		fixed = null;	// Already fixed to a node
	int		index;
	int		vis_x, vis_y;	// Where the rendering code placed it
    }

    /*
     * A container for physicalnode info.
     */
    private class PhysNode {
	String		pname;
	String		vname;
	String		type;		// Node type (garcia, mote, etc).
	boolean		mobile;		// A robot or a fixed node (mote).
	boolean		allocated;	// Node is allocated or free.
	int		radius;		// Radius of circle, in pixels
	int		size;		// Actual size, in pixels
        int		x, y;		// Current x,y coords in pixels

	/*
	 * These are formatted as strings to avoid doing conversons
	 * on the fly when the table is redrawn. Note, we have to
	 * convert from pixel coords (what the server sends) to meters.
	 */
	String		x_meters           = "";
	String		y_meters           = "";
	String		z_meters           = "";
	String		or_string          = "";
	int index;
    }

    /*
     * The actual object.
     */
    private class Selector extends JPanel {
	JLabel		TitleArea;
	// Indexed by the virtual name, points to VirtNode struct above.
	Dictionary	VirtNodes;
	// Indexed by the physical name, points to PhysNode struct above.
	Dictionary	PhysNodes;

	// The currently selected virtual node.
	VirtNode	CurrentVnode = null;
	// The currently selected physical node.
	PhysNode	CurrentPnode = null;

	/*
	 * A little class that creates an active map for clicking on.
	 */
	private class ActiveMap extends JLabel implements MouseListener {
	    int		width, height, xoff, yoff;
	    boolean     isvirtmap;
	    
	    public ActiveMap(ImageIcon icon, boolean virtmap) {
		super(icon);
		this.addMouseListener(this);

		width  = icon.getIconWidth();
		height = icon.getIconHeight();
		isvirtmap = virtmap;

		if (width < myWidth)
		    xoff = -((myWidth / 2) - (width / 2));
		if (height < (myHeight / 2))
		    yoff = ((myHeight / 2) / 2) - (height / 2);
	    }
	    public void mousePressed(MouseEvent e) {
		int button = e.getButton();
		int x      = e.getX() + xoff;
		int y      = e.getY() - yoff;

		System.out.println("Clicked on " + e.getX() + "," + e.getY());
		System.out.println("Clicked on " + x + "," + y);

		if (x > 0 && y > 0 && x <= width && y <= height)
		    Select(x, y, isvirtmap);
	    }
	    public void mouseReleased(MouseEvent e) {
	    }	

	    public void mouseEntered(MouseEvent e) {
	    }

	    public void mouseExited(MouseEvent e) {
	    }

	    public void mouseClicked(MouseEvent e) {
	    }

	    public void actionPerformed(ActionEvent e) {
	    }	
	}

	/*
	 * Find out which node was clicked on
	 */
	
        public Selector(URL expURL, URL phyURL) {
	    ActiveMap foo = new ActiveMap(getImageIcon(expURL), true);

	    /*
	     * The upper scrollpane holds the virtual experiment image.
	     */
	    JScrollPane scrollPane = new JScrollPane(foo);
	    scrollPane.setPreferredSize(new
		Dimension(myWidth, myHeight/2));
	    add(scrollPane);

	    /*
	     * Create a title area to use for displaying messages
	     */
	    TitleArea = new JLabel("Pick a node, any node");
	    add(TitleArea);

	    /*
	     * The lower scrollpane holds the physical node map image.
	     */
	    foo = new ActiveMap(getImageIcon(phyURL), false);
	    scrollPane = new JScrollPane(foo);
	    scrollPane.setPreferredSize(new Dimension(myWidth, myHeight/2));
	    add(scrollPane);

	    /*
	     * Get the virtnodes info.
	     */
	    if ((VirtNodes = GetVirtInfo()) == null) {
		MyDialog("GetVirtInfo",
			 "Failed to get virtnode list from server");
	    }

	    /*
	     * Get the physnodes info.
	     */
	    if ((PhysNodes = GetNodeInfo()) == null) {
		MyDialog("GetNodeInfo",
			 "Failed to get physical node list from server");
	    }
        }

	/*
	 * Made a selection. Figure out what it was.
	 */
	public void Select(int x, int y, boolean virtmap) {
	    if (virtmap == true) {
		// Set it back to null since a selection that does not
		// map to a node, is the same as a deselection.
		CurrentVnode = null;
		
		Enumeration e = VirtNodes.elements();

		while (e.hasMoreElements()) {
		    VirtNode vnode  = (VirtNode) e.nextElement();

		    if ((Math.abs(vnode.vis_y - y) < 10 &&
			 Math.abs(vnode.vis_x - x) < 10)) {
			CurrentVnode = vnode;
			break;
		    }
		}
		SetTitle();
	    }
	}

	/*
	 * Change the little message area text at the top of the applet.
	 */
	public void SetTitle() {
	    // Must be a space or else the label zaps away someplace.
	    TitleArea.setText(" ");
	    
	    if (CurrentVnode != null) {
		TitleArea.setText("Virtual node " +
				  CurrentVnode.vname + " selected");
	    }
	}
    }

    /*
     * Get the virtnode info and return a dictionary to the caller.
     */
    public Dictionary GetVirtInfo() {
	Dictionary	virtnodes = new Hashtable();
	BufferedReader  myreader  = OpenURL("virtinfo.php3", null);
	String		str;
	int		index = 0;
	int		min_x = 999999, min_y = 999999;
	int		max_x = 0, max_y = 0;

	if (myreader == null)
	    return null;

	try
	{
	    while ((str = myreader.readLine()) != null) {
		System.out.println(str);

		StringTokenizer	tokens = new StringTokenizer(str, ",");
		VirtNode	vnode  = new VirtNode();

		String vname = tokens.nextToken().trim();
		String fixed = tokens.nextToken().trim();
		int    vis_x = Integer.parseInt(tokens.nextToken().trim());
		int    vis_y = Integer.parseInt(tokens.nextToken().trim());

		if (fixed.length() == 0) {
		    fixed = null;
		}
		vnode.index = index++;
		vnode.vname = vname;
		vnode.fixed = fixed;
		vnode.pname = fixed;
		vnode.vis_x = vis_x;
		vnode.vis_y = vis_y;

		virtnodes.put(vname, vnode);

		if (vis_x < min_x) { min_x = vis_x; }
		if (vis_y < min_y) { min_y = vis_y; }
		if (vis_x > max_x) { max_x = vis_x; }
		if (vis_y > max_y) { max_y = vis_y; }
	    }
	    myreader.close();

	    System.out.println(min_x + "," + min_y+ "," +
			       max_x + "," + max_y + "," );

	    /*
	     * Now adjust for zoom and offset. This mirrors the calculation
	     * done in vis/render.in. Bogus, I know. But I do not want to
	     * change that part yet.
	     */
	    Enumeration e = virtnodes.elements();

	    while (e.hasMoreElements()) {
		VirtNode vnode  = (VirtNode) e.nextElement();

		vnode.vis_x = ((vnode.vis_x - min_x) * ZOOM) + X_ADJUST;
		vnode.vis_y = ((vnode.vis_y - min_y) * ZOOM) + Y_ADJUST;

		System.out.println(vnode.vname + " " + vnode.vis_x + "," + vnode.vis_y);
	    }
	}
	catch(Throwable th)
	{
	    th.printStackTrace();
	    return null;
	}
	return virtnodes;
    }

    /*
     * Get the physnode info and return a dictionary to the caller.
     */
    public Dictionary GetNodeInfo() {
	Dictionary	physnodes = new Hashtable();
	BufferedReader  myreader  = OpenURL("nodeinfo.php3",
					    "selector=1" +
					    "&floor=" + floor +
					    "&building=" + building);
	String		str;
	int		index = 0;

	if (myreader == null)
	    return null;

	try
	{
	    while ((str = myreader.readLine()) != null) {
		System.out.println(str);

		StringTokenizer	tokens = new StringTokenizer(str, ",");
		PhysNode	pnode  = new PhysNode();

		pnode.pname = tokens.nextToken().trim();
		pnode.type  = tokens.nextToken().trim();
		pnode.allocated =
		    tokens.nextToken().trim().compareTo("1") == 0;
		pnode.mobile    =
		    tokens.nextToken().trim().compareTo("1") == 0;
		pnode.size      = (int)
		    (Float.parseFloat(tokens.nextToken().trim()) *
				      pixels_per_meter);
		pnode.radius    = (int)
		    (Float.parseFloat(tokens.nextToken().trim()) *
				      pixels_per_meter);
		
		pnode.x         = Integer.parseInt(tokens.nextToken().trim());
		pnode.y         = Integer.parseInt(tokens.nextToken().trim());
		pnode.x_meters  =
		    FORMATTER.format(pnode.x / pixels_per_meter);
		pnode.y_meters  =
		    FORMATTER.format(pnode.y / pixels_per_meter);

		pnode.index = index++;
		physnodes.put(pnode.pname, pnode);
	    }
	    myreader.close();
	}
	catch(Throwable th)
	{
	    th.printStackTrace();
	    return null;
	}
	return physnodes;
    }

    /*
     * Open a URL, returning a Buffered reader for input. The caller will
     * close that reader. 
     */
    public BufferedReader OpenURL(String page, String arguments) {
	BufferedReader input = null;
	
	if (arguments == null)
	    arguments = "";
	else
	    arguments = "&" + arguments;
	
 	String urlstring = page
            + "?fromapplet=1"
	    + arguments
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

	    System.out.println("Opening URL: " + url.toString());	
	    
	    urlConn = url.openConnection();
	    urlConn.setDoInput(true);
	    urlConn.setUseCaches(false);
	    is = urlConn.getInputStream();
	    input = new BufferedReader(new InputStreamReader(is));
	}
	catch(Throwable th)
	{
	    th.printStackTrace();
	    return null;
	}
	return input;
    }

    /*
     * A thing that waits for an image to be loaded. 
     */
    public ImageIcon getImageIcon(URL url) {
	System.out.println("Opening URL: " + url.toString());	
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
	return new ImageIcon(img);
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
	int myHeight = 800;
	int myWidth  = 1000;
        final NodeSelect selector = new NodeSelect();
	try
	{
	    URL url = new URL("file:///tmp/robots-4.jpg");
	    selector.myWidth  = myWidth;
	    selector.myHeight = myHeight;
	    selector.init(true, url, url);
	}
	catch(Throwable th)
	{
	    th.printStackTrace();
	}
	
        Frame f = new Frame("Selector");
	
        f.addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent e) {System.exit(0);}
            public void windowDeiconified(WindowEvent e) { selector.start(); }
            public void windowIconified(WindowEvent e) { selector.stop(); }
        });
        f.add(selector);
        f.pack();
        f.setSize(new Dimension(myWidth, myHeight));
        f.setVisible(true);	
        f.show();
        selector.start();
    }
}
