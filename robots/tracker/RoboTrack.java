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
import javax.swing.table.AbstractTableModel;
import java.net.URL;
import java.lang.Math;
import java.util.*;
import java.text.*;
import java.io.*;
import java.net.URL;
import java.net.URLEncoder;
import java.net.URLConnection;

/*
 * Draw the floormap and put little dots on it. 
 */
public class RoboTrack extends JApplet {
    static Map map;
    static JTable maptable;
    static Image floorimage;
    boolean frozen = false;
    
    /*
     * The connection to boss that will provide robot location info.
     * When run interactively (from main) it is set to stdin (see below).
     */
    static InputStream is;

    public void init() {
	try
	{
	    URL urlServer = this.getCodeBase(), robopipe, floorurl;
	    String uid, auth, pipeurl, baseurl;
	    URLConnection uc;

	    /* Get our parameters then */
	    uid     = this.getParameter("uid");
	    auth    = this.getParameter("auth");
	    pipeurl = this.getParameter("pipeurl");
	    baseurl = this.getParameter("baseurl");

	    // form the URL that we use to get the background image
	    floorurl = new URL(urlServer,
			       baseurl
			       + "&nocookieuid="
			       + URLEncoder.encode(uid)
			       + "&nocookieauth="
			       + URLEncoder.encode(auth));

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
	 * Middle mouse button will stop/start the display.
	 */
	addMouseListener(new MouseAdapter() {
		public void mousePressed(MouseEvent e) {
		    if (frozen) {
			frozen = false;
			start();
		    } else {
			frozen = true;
			stop();
		    }
		}
	    });

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
     * A containter for coordinate information.
     */
    static class Robot {
        int x, y;			// Current x,y coords
	double or = 500.0;		// Current orientation 
	int dx, dy;			// Destination x,y coords
	double dor = 500.0;		// Destination orientation
	boolean gotdest = false;	// We have a valid destination
	double battery_voltage    = 500.0;
	double battery_percentage = 500.0;
	String pname, vname;		
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
	int DOT_RAD   = 14;
	// The length of the orientation stick, from the center of the circle.
	int STICK_LEN = 15;
	// The distance from center of dot to the label x,y.
	int LABEL_X   = -15;
	int LABEL_Y   = 18;
	
        public Map() {
        }

	/*
	 * Compute the endpoint of a line segment starting at the
	 * center of a circle (x,y) with orientation (angle).
	 */
	public int[] ComputeOrientationLine(int x, int y,
					    int dist, double angle) {
	    int retvals[] = new int[2];

	    retvals[0] = x + dist *
		(int) Math.cos(-(angle) * 3.1415926536 / 180.0);
	    retvals[1] = y + dist *
		(int) Math.sin(-(angle) * 3.1415926536 / 180.0);
    
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

	    str = tokens.nextToken().trim();
	    if (str.length() > 0)
		robbie.or = Float.parseFloat(str);

	    str = tokens.nextToken().trim();
	    if (str.length() > 0) {
		robbie.dx  = Integer.parseInt(str);
		robbie.dy  = Integer.parseInt(tokens.nextToken().trim());
		robbie.dor = Float.parseFloat(tokens.nextToken().trim());
		robbie.gotdest = true;
	    }
	    else {
		// Consume next two tokens.
		str = tokens.nextToken();
		str = tokens.nextToken();
	    }

	    str = tokens.nextToken().trim();
	    if (str.length() > 0)
		robbie.battery_percentage = Float.parseFloat(str);
	    str = tokens.nextToken().trim();
	    if (str.length() > 0)
		robbie.battery_voltage = Float.parseFloat(str);
	}

	/*
	 * Draw a robot, which is either the real one or a destination one.
	 */
	public void drawRobot(Graphics2D g2,
			      int x, int y, double or,
			      String label, boolean dest) {
	    /*
	     * An allocated robot is a filled circle.
	     */
	    g2.setColor(Color.green);

	    if (!dest)
		g2.fillOval(x - (DOT_RAD/2), y - (DOT_RAD/2),
			    DOT_RAD, DOT_RAD);
	    else 
		g2.drawOval(x - (DOT_RAD/2), y - (DOT_RAD/2),
			    DOT_RAD, DOT_RAD);
	    
	    /*
	     * If there is a orientation, add an orientation stick
	     * to it.
	     */
	    if (or != 500.0) {
		int endpoint[] = ComputeOrientationLine(x, y, STICK_LEN, or);
		
		g2.drawLine(x, y, endpoint[0], endpoint[1]);
	    }

	    /*
	     * Draw a label for the robot, either above or below,
	     * depending on where the orientation stick was.
	     */
	    int lx = x + LABEL_X;
	    int ly = y + LABEL_Y;

	    if ((or > 180.0 && or < 360.0) ||
		(or < 0.0   && or > -180.0))
		ly = y - (LABEL_Y/2);

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
	    Enumeration e = robots.elements();

	    while (e.hasMoreElements()) {
		Robot robbie  = (Robot)e.nextElement();

		int x = robbie.x;
		int y = robbie.y;

		drawRobot(g2, x, y, robbie.or, robbie.pname, false);

		/*
		 * Okay, if the robot has a destination, draw that too
		 * but as a hollow circle.
		 */
		if (robbie.gotdest) {
		    int dx = robbie.dx;
		    int dy = robbie.dy;
		    
		    drawRobot(g2, dx, dy, robbie.dor, robbie.pname, true);

		    /*
		     * And draw a light grey line from source to dest.
		     */
		    g2.setColor(Color.gray);
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
	    
            while (thread == me) {
		try
		{
		    while (null != ((str = input.readLine()))) {
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
    
    static class MyTableModel extends AbstractTableModel {
        private String[] columnNames = {"Pname", "Vname",
	                                "X", "Y", "O",
	                                "DX", "DY", "DO",
					"Battery %", "Voltage"
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
	    case 2: return "" + robbie.x;
	    case 3: return "" + robbie.y;
	    case 4: return (robbie.or != 500.0 ? "" + robbie.or : "");
	    case 5: return (robbie.gotdest ? "" + robbie.dx  : "");
	    case 6: return (robbie.gotdest ? "" + robbie.dy  : "");
	    case 7: return (robbie.dor != 500.0 ? "" + robbie.dor : "");
	    case 8: return (robbie.battery_percentage != 500.0 ?
			    "" + robbie.battery_percentage : "");
	    case 9: return (robbie.battery_voltage != 500.0 ?
			    "" + robbie.battery_voltage : "");
	    }
	    return "Foo";
        }

       /*
	* JTable uses this method to determine the default renderer/
	* editor for each cell.  If we didn't implement this method,
	* then the last column would contain text ("true"/"false"),
	* rather than a check box.
	*/
        public Class getColumnClass(int c) {
            return getValueAt(0, c).getClass();
        }

        /*
         * Don't need to implement this method unless your table's
         * editable.
         */
        public boolean isCellEditable(int row, int col) {
	    return false;
        }
    }

    public static void main(String argv[]) {
        final RoboTrack robomap = new RoboTrack();
	try
	{
	    URL url = new URL("file://robots-4.jpg");
	    robomap.init();
	    robomap.is = System.in;
	    robomap.floorimage = robomap.getImage(url);
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
