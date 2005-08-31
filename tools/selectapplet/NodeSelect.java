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
    private boolean		shelled = false;
    private Selector		selector;
    int				myHeight, myWidth;
    String			uid, auth, pid, eid;
    double			pixels_per_meter = 100.0;
    URL				urlServer;
    static final DecimalFormat  FORMATTER = new DecimalFormat("0.00");
    static final int            INCH =
	Toolkit.getDefaultToolkit().getScreenResolution();
    
    /* From the floormap generation code. */
    int X_ADJUST = 60;
    int Y_ADJUST = 60;

    // The font width/height adjustment, for drawing labels.
    int FONT_HEIGHT = 16;
    int FONT_WIDTH  = 10;
    Font OurFont    = null;
    Font BoldFont   = null;
    Font RulerFont  = null;

    // The width of the list boxes.
    int LISTBOX_WIDTH = 150;

    // For zooming ...
    double scale = 1.0;

    // rulers
    int HORIZONTAL = 0;
    int VERTICAL   = 1;
    
    /*
     * A little class to hold info we need to get data from the server
     * about buildings, floors, nodes.
     */
    private class FloorInfo {
	String		building;
	String		floor;
	URL		icon_url;
    }

    public void init() {
	String		building;
	
	/*
	 * An array of floorinfo thingies.
	 */
	int		floorcount;
	FloorInfo	floorinfo[] = null;

	try
	{
	    /* Get our parameters */
	    uid        = this.getParameter("uid");
	    auth       = this.getParameter("auth");
	    pid        = this.getParameter("pid");
	    eid        = this.getParameter("eid");
	    floorcount = Integer.parseInt(this.getParameter("floorcount"));
	    building   = this.getParameter("building");
	    pixels_per_meter = Double.parseDouble(this.getParameter("ppm"));

	    // Now we can create this ...
	    floorinfo = new FloorInfo[floorcount];

	    for (int i = 0; i < floorcount; i++) {
		String paramname  = "floor_" + i;
		String floorname;

		if ((floorname = this.getParameter(paramname)) == null)
		    continue;

		floorinfo[i]          = new FloorInfo();
		floorinfo[i].floor    = floorname;
		floorinfo[i].building = building;

		// form the URL that we use to get the physical layout icon
		floorinfo[i].icon_url = new URL(this.getCodeBase(),
						"flooricon.php3?building="
						+ building
						+ "&floor="
						+ floorname
						+ "&nocookieuid="
						+ URLEncoder.encode(uid)
						+ "&nocookieauth="
						+ URLEncoder.encode(auth));
	    }
	}
	catch(Throwable th)
	{
	    th.printStackTrace();
	}
	init(false, floorinfo);
    }

    public void start() {
    }
  
    public void stop() {
    }

    // For the benefit of running from the shell for testing.
    public void init(boolean fromshell, URL url) {
	FloorInfo floorinfo[] = new FloorInfo[3];

	floorinfo[0] = new FloorInfo();
	floorinfo[0].floor    = "1";
	floorinfo[0].building = "MEB";
	floorinfo[0].icon_url = url;

	floorinfo[1] = new FloorInfo();
	floorinfo[1].floor    = "2";
	floorinfo[1].building = "MEB";
	floorinfo[1].icon_url = url;

	floorinfo[2] = new FloorInfo();
	floorinfo[2].floor    = "3";
	floorinfo[2].building = "MEB";
	floorinfo[2].icon_url = url;

	init(fromshell, floorinfo);
    }
	    
    public void init(boolean fromshell, FloorInfo floorinfo[]) {
	shelled = fromshell;

	if (!shelled) {
	    Dimension appletSize = this.getSize();
		
	    myHeight = appletSize.height;
	    myWidth  = appletSize.width;
	}
	OurFont   = new Font("Helvetica", Font.BOLD, 12);
	BoldFont  = new Font("Helvetica", Font.BOLD, 20);
	RulerFont = new Font("SansSerif", Font.PLAIN, 10);

	/*
	 * Now add my one object to the contentpane.
	 */
	selector = new Selector(floorinfo);
	getContentPane().setLayout(new BoxLayout(getContentPane(),
						 BoxLayout.Y_AXIS));
	getContentPane().add(selector);
    }

    /*
     * A container for stuff that is common to both virtnodes and physnodes.
     */
    private class Node implements Comparable {
	String		vname;
	String		pname;
	int		radius = 15;	// Radius of circle, in pixels
	int		size   = 20;	// Actual size, in pixels
        int		x, y;		// Current x,y coords in pixels
	int		listindex;	// Where it sits in its listbox.
	boolean		picked;		// Node is within a selection
	boolean		mouseover;	// currently mousedover
	Object		map;		// Pointer back to activemap.

	/*
	 * These are formatted as strings to avoid doing conversons
	 * on the fly when the table is redrawn. Note, we have to
	 * convert from pixel coords (what the server sends) to meters.
	 */
	String		x_meters           = "";
	String		y_meters           = "";
	int		index;

	public void setX(int x) {
	    this.x = x;
	    this.x_meters = FORMATTER.format(x / pixels_per_meter);
	}
	public void setY(int y) {
	    this.y = y;
	    this.y_meters = FORMATTER.format(y / pixels_per_meter);
	}

	public int scaledX() {
	    return (int) (x * scale);
	}
	public int scaledY() {
	    return (int) (y * scale);
	}

	// Comparator interface for sorting below.
	public int compareTo(Object o) {
	    Node other = (Node) o;

	    return this.pname.compareTo(other.pname);
	}
    }

    /*
     * A container for virtual node info.
     */
    private class VirtNode extends Node {
	String		fixed = null;	// Already fixed to a node
	
	public String toString() {
	    return this.vname;
	}
    }

    /*
     * A container for physical node info.
     */
    private class PhysNode extends Node {
	String		type;		// Node type (garcia, mote, etc).
	boolean		mobile;		// A robot or a fixed node (mote).
	boolean		allocated;	// Node is allocated or free.
	boolean		selected;	// Node has been selected for inclusion

	public String toString() {
	    String label = this.pname;

	    // This is to avoid writing my own JList cell renderer!
	    if (this.vname != null)
		label = label + " (" + this.vname +")";

	    return label;
	}
    }

    /*
     * A utility class for menu items. Associate a handler with each menu
     * item, and store those in a dictionary so we can find them when the
     * mousehandler fires.
     */
    private class MenuHandler {
	JMenuItem menuitem;
	
	public MenuHandler() {
	}
	
	public MenuHandler(JMenuItem menuitem) {
	    this.menuitem = menuitem;
	}

	public void enable(boolean which) {
	    this.menuitem.setEnabled(which);
	}
	
	public void handler(String action) {
	    MyDialog("MenuHandler", "No handler specified!");
	}
    }

    /*
     * The actual object.
     */
    private class Selector extends JPanel implements ActionListener,
                                                     MouseListener
    {
	JLabel		MessageArea;
	// Indexed by the virtual name, points to VirtNode struct above.
	Dictionary	VirtNodes;
	// Indexed by the physical name, points to PhysNode struct above.
	Dictionary	AllPhysNodes = new Hashtable();
	// A list of the ActiveMaps.
	Vector		PhysMaps = new Vector(10, 10);
	// For the menu items.
	Dictionary	MenuHandlers = new Hashtable();

	// The lists boxes, one for all nodes and one for selected nodes.
	JList		AllPhysList;
	JList           SelectList;

	// The physmap scroll pane.
	JScrollPane	PhysMapScrollPane;
	ScrollablePanel	PhysMapPanel;

	// The root menu.
	JPopupMenu	RootPopupMenu;
	// The node menu.
	JPopupMenu	NodePopupMenu;
	// For the node menu, need to know what the node was!
	Node		PopupNode;

	// Last centering target.
	Rectangle	LastCenter = null;

	private class ScrollablePanel extends JPanel
	    implements Scrollable, MouseMotionListener {
	    private int maxUnitIncrement = 5;

	    public ScrollablePanel() {
		super();

		// enable synthetic drag events		
		setAutoscrolls(true);
		addMouseMotionListener(this);
	    }
	    private Point lastpoint = null;
	    // I do not want to be heap allocating points every event!
	    private Point Ptemp = new Point();
	    private Point Pview = new Point();
	    
	    public void mouseReleased(MouseEvent e) {
		lastpoint = null;
	    }
	    // Methods required by MouseMotionListener
	    public void mouseMoved(MouseEvent e) {
	    }
	    public void mouseDragged(MouseEvent e) {
		// Current view position. I hate to heap allocate a point.
		Rectangle view = PhysMapScrollPane.getViewport().getViewRect();

		// Map the current event to screen coordinates.
		Ptemp.x = e.getX();
		Ptemp.y = e.getY();
		javax.swing.SwingUtilities.convertPointToScreen(Ptemp,
							e.getComponent());

		// So we can track how far the user moved the mouse since
		// the last event came in.
		if (lastpoint == null)
		    lastpoint = new Point(Ptemp.x, Ptemp.y);

		// Move the current view point in the direction the user moved.
		view.x = view.x + (lastpoint.x - Ptemp.x);
		view.y = view.y + (lastpoint.y - Ptemp.y);

		//System.out.println("view 1 " + view);
		//System.out.println("x,y    " + getWidth() +
		//                   "," + getHeight());

		// But make sure we do not scroll it off screen.
		if (view.x + view.width > getWidth())
		    view.x = getWidth() - view.width;
		if (view.y + view.height > getHeight())
		    view.y = getHeight() - view.height;
		if (view.x < 0)
		    view.x = 0;
		if (view.y < 0)
		    view.y = 0;

		// Save for next event.
		lastpoint.x = Ptemp.x;
		lastpoint.y = Ptemp.y;

		// System.out.println("view 2 " + view);

		// And move the viewport. I use setviewposition instead of
		// scrollrect cause it appears to do (exactly) what I want.
		Pview.x = view.x;
		Pview.y = view.y;
		PhysMapScrollPane.getViewport().setViewPosition(Pview);
		repaint();
	    }
	    
	    // Methods required by the Scrollable.
	    public Dimension getPreferredSize() {
		return super.getPreferredSize();
	    }
	    public Dimension getPreferredScrollableViewportSize() {
		return getPreferredSize();
	    }
	    public boolean getScrollableTracksViewportWidth() {
		return false;
	    }
	    public boolean getScrollableTracksViewportHeight() {
		return false;
	    }
	    public void setMaxUnitIncrement(int pixels) {
		maxUnitIncrement = pixels;
	    }
	    public int getScrollableBlockIncrement(Rectangle visibleRect,
						   int orientation,
						   int direction) {
		if (orientation == SwingConstants.HORIZONTAL)
		    return visibleRect.width - maxUnitIncrement;
		else
		    return visibleRect.height - maxUnitIncrement;
	    }
	    public int getScrollableUnitIncrement(Rectangle visibleRect,
						  int orientation,
						  int direction) {
		// Get the current position.
		int currentPosition = 0;
		if (orientation == SwingConstants.HORIZONTAL)
		    currentPosition = visibleRect.x;
		else
		    currentPosition = visibleRect.y;

		// Return the number of pixels between currentPosition
		// and the nearest tick mark in the indicated direction.
		if (direction < 0) {
		    int newPosition = currentPosition -
			(currentPosition / maxUnitIncrement) *
			maxUnitIncrement;
		    return (newPosition == 0) ? maxUnitIncrement : newPosition;
		}
		else {
		    return ((currentPosition / maxUnitIncrement) + 1)
			* maxUnitIncrement - currentPosition;
		}
	    }
	}

	/*
	 * This is a subclass of Icon, that draws the nodes in where they
	 * are supposed to be, superimposed over the base icon.
	 */
        private class FloorIcon implements Icon {
	    // The base image data (floormap).
	    ImageIcon	myicon;
	    // A list of physical nodes on this floor.
	    Dictionary	NodeList;
	    // Cache some values.
	    int		iconWidth, iconHeight;

	    public FloorIcon(ImageIcon icon, Dictionary nodelist) {
		myicon    = icon;
		NodeList  = nodelist;

		iconWidth  = (int) (icon.getIconWidth() * scale);
		iconHeight = (int) (icon.getIconHeight() * scale);
	    }
	    public int getIconHeight() {
		return iconHeight;
	    }
	    public int getIconWidth() {
		return iconWidth;
	    }
	    public void rescale() {
		iconWidth  = (int) (myicon.getIconWidth() * scale);
		iconHeight = (int) (myicon.getIconHeight() * scale);
	    }

	    /*
	     * The paint method sticks the little dots in.
	     * Note X,Y in here do not need to be scaled since the G2
	     * will apply the scaling to all the coords used.
	     */
	    public void paintIcon(Component c, Graphics g, int x, int y) {
		Graphics2D g2 = (Graphics2D) g;
		g2.scale(scale, scale);
		myicon.paintIcon(c, g2, x, y);

		/*
		 * Draw all the nodes on this map.
		 */
		Enumeration e = NodeList.elements();

		while (e.hasMoreElements()) {
		    PhysNode	node  = (PhysNode) e.nextElement();
		    String      label = node.pname;

		    /*
		     * Draw a little circle where the node lives.
		     */
		    if (node.allocated)
			g.setColor(Color.red);
		    else if (node.picked)
			g.setColor(Color.yellow);
		    else if (node.selected)
			g.setColor(Color.blue);
		    else
			g.setColor(Color.green);
		    
		    g.fillOval(node.x - node.radius,
			       node.y - node.radius,
			       node.radius * 2,
			       node.radius * 2);

		    /*
		     * Draw a label below the dot.
		     */
		    int lx  = node.x - ((FONT_WIDTH * label.length()) / 2);
		    int ly  = node.y + node.radius + FONT_HEIGHT;

		    if (node.mouseover) {
			g.setColor(Color.blue);
			g2.setFont(BoldFont);
		    }
		    else {
			g.setColor(Color.black);
			g2.setFont(OurFont);
		    }
		    g.drawString(label, lx, ly);
		}
		// Restore Graphics object
		//g2.scale(1.0 - scale, 1.0 - scale);
	    }
	}	    

	/*
	 * A little class that creates an active map for clicking on.
	 */
	private class ActiveMap extends JLabel
	    implements MouseListener, MouseMotionListener
	{
	    int		width, height, xoff = 0, yoff = 0;
	    boolean     isvirtmap;
	    Dictionary  NodeList;
	    FloorIcon   flooricon;
	    Node	mousedover = null;
	    
	    public ActiveMap(FloorIcon flooricon, Dictionary nodelist,
			     int thiswidth, int thisheight, boolean virtmap) {
		super(flooricon);
		this.flooricon = flooricon;
		this.addMouseListener(this);
		this.addMouseMotionListener(this);

		NodeList  = nodelist;
		isvirtmap = virtmap;
		width  = flooricon.getIconWidth();
		height = flooricon.getIconHeight();

		// Set the back pointer.
		Enumeration e = nodelist.elements();
		while (e.hasMoreElements()) {
		    Node node = (Node) e.nextElement();

		    node.map = this;
		}

		if (false) {
		if (width < thiswidth)
		    xoff = -((thiswidth / 2) - (width / 2));
		if (height < (thisheight / 2))
		    yoff = ((thisheight / 2) / 2) - (height / 2);
		}
	    }
	    public int getHeight() {
		return height;
	    }
	    public int getWidth() {
		return width;
	    }

	    public void rescale() {
		flooricon.rescale();
		// Pick up new scale.
		width  = flooricon.getIconWidth();
		height = flooricon.getIconHeight();

		this.setPreferredSize(new Dimension(width, height));
		this.revalidate();
	    }
	    public void mousePressed(MouseEvent e) {
		int button     = e.getButton();
		int modifier   = e.getModifiersEx();
		int x          = e.getX() + xoff;
		int y          = e.getY() - yoff;
		PhysNode pnode = null;

		System.out.println("Clicked on " + e.getX() + "," + e.getY());
		System.out.println("Clicked on " + x + "," + y);

		if (x > 0 && y > 0 && x <= width && y <= height)
		    pnode = (PhysNode)
			FindNode(this, (int)(x / scale), (int)(y / scale));
		
		if (!e.isPopupTrigger()) {
		    if (pnode != null) {
			if (!pnode.allocated)
			    SelectNode(pnode, modifier);
			return;
		    }
		}
		// Forward to outer event handler, including the node
		selector.doPopupMenu(e, pnode);
	    }
	    public void mouseReleased(MouseEvent e) {
		PhysMapPanel.mouseReleased(e);
	    }	
	    public void mouseEntered(MouseEvent e) {
	    }
	    public void mouseExited(MouseEvent e) {
	    }
	    public void mouseClicked(MouseEvent e) {
	    }
	    // MouseMotionListener interface methods.
	    public void mouseDragged(MouseEvent e) {
		// Drag panel on left-click and hold.
		if ((e.getModifiersEx() & InputEvent.BUTTON1_DOWN_MASK) != 0)
		    PhysMapPanel.mouseDragged(e);
	    }
	    public void mouseMoved(MouseEvent e) {
		int x        = e.getX();
		int y        = e.getY();
		Node node    = null;

		node = FindNode(this, (int)(x / scale), (int)(y / scale));

		if (node != null) {
		    node.mouseover = true;

		    if (mousedover != null) {
			if (node == mousedover)
			    return;
			mousedover.mouseover = false;
		    }
		    mousedover = node;
		    repaint();
		    return;
		}
		// Not over anything; be sure to clear the current one.
		if (mousedover != null) {
		    mousedover.mouseover = false;
		    mousedover = null;
		    repaint();
		}
	    }
	}

        public Selector(FloorInfo floorinfo[]) {
	    ActiveMap   activemap;
	    Dictionary  PhysNodes;
	    
	    /*
	     * We leave room on the right for the two list boxes.
	     * The height depends on whether there is a virtual map box.
	     */
	    int mapwidth  = myWidth - LISTBOX_WIDTH;
	    int mapheight = myHeight;

	    /*
	     * Okay, need some panels so that we do not have to do 
	     * specific layout ourselves.
	     *
	     * Create one panel to hold the map(s), on the left side.
	     * Create another panel to hold the list boxes, on the right side.
	     */
	    JPanel  MapPanel  = new JPanel(true);
	    JPanel  ListPanel = new JPanel(true);

	    // Both of these panels lay things out vertically.
	    MapPanel.setLayout(new BoxLayout(MapPanel, BoxLayout.Y_AXIS));
	    ListPanel.setLayout(new BoxLayout(ListPanel, BoxLayout.PAGE_AXIS));
	    ListPanel.setPreferredSize(new Dimension(LISTBOX_WIDTH,
						     mapheight));

	    /*
	     * However, we want the above two panels to be laid out
	     * horizonally within the surrounding panel.
	     */
	    this.setLayout(new BoxLayout(this, BoxLayout.X_AXIS));

	    /*
	     * Create the list boxes in their panel, since we want to
	     * add the nodes as we get them from the web server. Note
	     * that they need to go into their own Scroll panes so they
	     * get scrollbars. 
	     */
	    AllPhysList = new JList(new DefaultListModel());
	    SelectList  = new JList(new DefaultListModel());

	    // Set the selection listener so we know what the user picked.
	    ListSelectionHandler mylsh = new ListSelectionHandler();
	    AllPhysList.getSelectionModel().addListSelectionListener(mylsh);
	    SelectList.getSelectionModel().addListSelectionListener(mylsh);

	    // So we get mouse events in the lists panels.
	    AllPhysList.addMouseListener(this);
	    SelectList.addMouseListener(this);
 
	    AllPhysList.setLayoutOrientation(JList.VERTICAL);
	    SelectList.setLayoutOrientation(JList.VERTICAL);

	    // Must enclose in a scrollpane to get scrollbars.
	    JScrollPane ToplistScroller = new JScrollPane(AllPhysList);
	    JScrollPane BotlistScroller = new JScrollPane(SelectList);

	    // add a label for the upper list box
	    ListPanel.add(new JLabel("Available Nodes"));

	    // and add the upper list box.
	    ListPanel.add(ToplistScroller);

	    // add a label for the lower list box
	    ListPanel.add(new JLabel("Assigned Nodes"));

	    // And add the lower listbox.
	    ListPanel.add(BotlistScroller);

	    /*
	     * Create a message area to use for displaying messages
	     */
	    MessageArea = new JLabel("Pick a node, any node");
	    MapPanel.add(MessageArea);

	    /*
	     * And a panel to put all the little scroll panes in.
	     */
	    PhysMapPanel = new ScrollablePanel();
	    
	    /*
	     * Specify vertical placement of components inside this panel
	     */
	    PhysMapPanel.setLayout(new BoxLayout(PhysMapPanel,
						  BoxLayout.Y_AXIS));

	    /*
	     * Get the physnodes info. Each submap is added to a large
	     * panel, which we then add to a scroll pane.
	     */
	    for (int i = 0; i < floorinfo.length; i++) {
		String		floor    = floorinfo[i].floor;
		String		building = floorinfo[i].building;
		URL		icon_url = floorinfo[i].icon_url;
		
		if ((PhysNodes = GetNodeInfo(building, floor)) == null) {
		    MyDialog("GetNodeInfo",
			     "Failed to get physical node list from server");
		}

		Enumeration e = PhysNodes.elements();
		while (e.hasMoreElements()) {
		    PhysNode pnode = (PhysNode) e.nextElement();

		    // Add to global Dict so we can easily find it.
		    AllPhysNodes.put(pnode.pname, pnode);
		}

		/*
		 * Stick in a title to let the user know what floor/building.
		 */
		JLabel TitleArea =
		    new JLabel(building + ", " + floor);
		TitleArea.setAlignmentX(Component.CENTER_ALIGNMENT);
		PhysMapPanel.add(TitleArea);

		activemap = new
		    ActiveMap(new FloorIcon(getImageIcon(icon_url), PhysNodes),
			      PhysNodes, mapwidth, mapheight, false);
		activemap.setAlignmentX(Component.CENTER_ALIGNMENT);
		PhysMapPanel.add(activemap);

		// Remember them all in a list.
		PhysMaps.insertElementAt(activemap, i);
	    }

	    /*
	     * Get the virtnodes info. 
	     */
	    if (pid != null && eid != null) {
		if ((VirtNodes = GetVirtInfo()) == null) {
		    MyDialog("GetVirtInfo",
			     "Failed to get virtnode list from server");
		}

		/*
		 * Go through the virtlist, and assign any fixed nodes. Fixed
		 * nodes start out as being selected; it is up to the user to
		 * unselect them.
		 */
		Enumeration e = VirtNodes.elements();
		while (e.hasMoreElements()) {
		    VirtNode vnode = (VirtNode) e.nextElement();

		    if (vnode.fixed != null) {
			PhysNode pnode = (PhysNode)
			    AllPhysNodes.get(vnode.fixed);

			if (pnode != null) {
			    pnode.vname    = vnode.vname;
			    pnode.selected = true;
			}
		    }
		}
	    }
	    
	    /*
	     * Now we can assign the nodes to one of the listboxes. "fixed"
	     * nodes start out in the selected list, as mentioned above.
	     */
	    Enumeration a = AllPhysNodes.elements();
	    
	    while (a.hasMoreElements()) {
		DefaultListModel model;
		PhysNode	 pnode = (PhysNode) a.nextElement();

		if (pnode.allocated)
		    continue;

		if (pnode.selected)
		    model = (DefaultListModel) SelectList.getModel();
		else
		    model = (DefaultListModel) AllPhysList.getModel();
		
		model.addElement(pnode);
		// Get back its index so we know where it landed.
		pnode.listindex = model.indexOf(pnode);
	    }

	    /*
	     * We need to force the physmappanel to layout now, so we
	     * figure out how big the ruler bars need to be. If we do not
	     * do that, the rulers will be 0 sized, and really hard to see!
	     */
	    PhysMapPanel.getLayout().layoutContainer(PhysMapPanel);
	    Dimension physmapdim =
		PhysMapPanel.getLayout().minimumLayoutSize(PhysMapPanel);

	    /*
	     * Create a surrounding scroll pane.   
	     */
	    PhysMapScrollPane = new JScrollPane(PhysMapPanel);
	    PhysMapScrollPane.setPreferredSize(new Dimension(mapwidth,
							     mapheight));

	    // Create the row and column headers (rulers, sortof).
	    Ruler columnView = new Ruler(HORIZONTAL);
	    Ruler rowView    = new Ruler(VERTICAL);

	    // Make sure the rulers know how big they need to be.
	    columnView.setPreferredWidth(physmapdim.width);
	    rowView.setPreferredHeight(physmapdim.height);

	    PhysMapScrollPane.setColumnHeaderView(columnView);
	    PhysMapScrollPane.setRowHeaderView(rowView);

	    // Now we can add the scrollpane.
	    MapPanel.add(PhysMapScrollPane);

	    // Add add our two inner panels to the outer panel
	    add(MapPanel);
	    add(ListPanel);

	    /*
	     * Okay, create the root menu.
	     */
	    RootPopupMenu = new JPopupMenu("Root Menu");
	    JMenuItem menuitem = new JMenuItem("    Root Menu    ");
	    RootPopupMenu.add(menuitem);
	    RootPopupMenu.addSeparator();

	    menuitem = new JMenuItem("Add selected nodes to list");
	    menuitem.setActionCommand("AddNodes");
	    menuitem.addActionListener(this);
	    MenuHandlers.put("AddNodes", new AddDelNodeMenuHandler(menuitem));
	    RootPopupMenu.add(menuitem);

	    menuitem = new JMenuItem("Remove selected nodes from list");
	    menuitem.setActionCommand("DelNodes");
	    menuitem.addActionListener(this);
	    MenuHandlers.put("DelNodes", new AddDelNodeMenuHandler(menuitem));
	    RootPopupMenu.add(menuitem);

	    menuitem = new JMenuItem("Create NS File");
	    menuitem.setActionCommand("NSFile");
	    menuitem.addActionListener(this);
	    MenuHandlers.put("NSFile", new NSFileMenuHandler(menuitem));
	    RootPopupMenu.add(menuitem);

	    RootPopupMenu.addSeparator();
	    if (pid != null) {
		menuitem = new JMenuItem("Clear Virtual Node Names");
		menuitem.setActionCommand("ClearVirt");
		menuitem.addActionListener(this);
		MenuHandlers.put("ClearVirt",
				 new ClearVirtMenuHandler(menuitem));
		RootPopupMenu.add(menuitem);
	    }
	    menuitem = new JMenuItem("Zoom In");
	    menuitem.setActionCommand("ZoomIn");
	    menuitem.addActionListener(this);
	    MenuHandlers.put("ZoomIn", new ZoomMenuHandler(menuitem));
	    RootPopupMenu.add(menuitem);

	    menuitem = new JMenuItem("Zoom Out");
	    menuitem.setActionCommand("ZoomOut");
	    menuitem.addActionListener(this);
	    MenuHandlers.put("ZoomOut", new ZoomMenuHandler(menuitem));
	    RootPopupMenu.add(menuitem);

	    menuitem = new JMenuItem("Recenter");
	    menuitem.setActionCommand("Recenter");
	    menuitem.addActionListener(this);
	    MenuHandlers.put("Recenter", new RecenterMenuHandler(menuitem));
	    RootPopupMenu.add(menuitem);

	    menuitem = new JMenuItem("Reset Zoom/Center");
	    menuitem.setActionCommand("Reset");
	    menuitem.addActionListener(this);
	    MenuHandlers.put("Reset", new ZoomMenuHandler(menuitem));
	    RootPopupMenu.add(menuitem);

	    RootPopupMenu.addSeparator();
	    menuitem = new JMenuItem("Close This Menu");
	    RootPopupMenu.add(menuitem);
	    RootPopupMenu.setBorder(
			BorderFactory.createLineBorder(Color.black));

	    /*
	     * And now create the node menu.
	     */
	    NodePopupMenu = new JPopupMenu("Node Context Menu");
	    menuitem = new JMenuItem("    Node Menu    ");
	    NodePopupMenu.add(menuitem);
	    NodePopupMenu.addSeparator();

	    menuitem = new JMenuItem("Set Virtual Name");
	    menuitem.setActionCommand("SetVname");
	    menuitem.addActionListener(this);
	    MenuHandlers.put("SetVname", new SetVnameMenuHandler(menuitem));
	    NodePopupMenu.add(menuitem);

	    NodePopupMenu.addSeparator();
	    menuitem = new JMenuItem("Add selected nodes to list");
	    menuitem.setActionCommand("AddNodes");
	    menuitem.addActionListener(this);
	    MenuHandlers.put("NodeMenu_AddNodes",
			     new AddDelNodeMenuHandler(menuitem));
	    NodePopupMenu.add(menuitem);

	    menuitem = new JMenuItem("Remove selected nodes from list");
	    menuitem.setActionCommand("DelNodes");
	    menuitem.addActionListener(this);
	    MenuHandlers.put("NodeMenu_DelNodes",
			     new AddDelNodeMenuHandler(menuitem));
	    NodePopupMenu.add(menuitem);
	    NodePopupMenu.addSeparator();

	    menuitem = new JMenuItem("Close This Menu");
	    NodePopupMenu.add(menuitem);
	    NodePopupMenu.setBorder(
			BorderFactory.createLineBorder(Color.black));
        }

	// Methods required by MouseListener interface.
	public void mousePressed(MouseEvent e) {
	    /*
	     * We know we came from one the JList boxes. Figure out if the
	     * click was over a cell of a node. If so, pass that along to
	     * doPopupMenu so that we get a node context menu.
	     */
	    JList	jlist      = (JList) e.getSource();
	    Point	P          = new Point(e.getX(), e.getY());
	    int		cellnum    = jlist.locationToIndex(P);
	    Rectangle   cellbounds = jlist.getCellBounds(cellnum, cellnum);
	    Node	thisnode   = null;

	    if (cellbounds != null && cellbounds.contains(P)) {
		DefaultListModel   lm = (DefaultListModel) jlist.getModel();

		thisnode = (Node) lm.getElementAt(cellnum);
	    }
	    
	    // Only forward popup for now. Needs more thought.
	    if (e.isPopupTrigger())
		doPopupMenu(e, thisnode);
	}
	public void mouseReleased(MouseEvent e) {
	}	
	public void mouseEntered(MouseEvent e) {
	}
	public void mouseExited(MouseEvent e) {
	}
	public void mouseClicked(MouseEvent e) {
	}

	/*
	 * Clear the selection.
	 */
	public void ClearSelection() {
	    int		index;
	    
	    for (index = 0; index < PhysMaps.size(); index++) {
		ActiveMap map = (ActiveMap) PhysMaps.elementAt(index);
		Enumeration e = map.NodeList.elements();

		while (e.hasMoreElements()) {
		    Node node  = (Node) e.nextElement();

		    node.picked = false;
		}
	    }
	    AllPhysList.clearSelection();
	    SelectList.clearSelection();
	    repaint();
	}

	/*
	 * Made a selection on the map. Figure out what it was and return.
	 * Note that X,Y come in already scaled.
	 */
	public Node FindNode(ActiveMap Map, int x, int y) {
	    /*
	     * Find the node.
	     */
	    Node	curnode = null;
	    Enumeration e = Map.NodeList.elements();

	    while (e.hasMoreElements()) {
		Node node  = (Node) e.nextElement();

		if ((Math.abs(node.y - y) < 10 &&
		     Math.abs(node.x - x) < 10)) {
		    curnode = node;
		    break;
		}
	    }
	    return curnode;
	}

	/*
	 * Select the node, once we have found it.
	 */
	public void SelectNode(Node curnode, int modifier) {
	    // Clear current selection if no modifier.
	    if ((modifier &
		 (InputEvent.SHIFT_DOWN_MASK|InputEvent.CTRL_DOWN_MASK))
		== 0) {
		ClearSelection();
	    }
	    curnode.picked = true;
	    
	    // Highlight in the appropriate listbox.
	    PhysNode pnode = (PhysNode) curnode;
		
	    if (pnode.selected) {
		SelectList.addSelectionInterval(pnode.listindex,
						pnode.listindex);
		ZapList(SelectList, pnode.listindex);
	    }
	    else {
		AllPhysList.addSelectionInterval(pnode.listindex,
						 pnode.listindex);
		ZapList(AllPhysList, pnode.listindex);
	    }

	    // Say something in the message area at the top.
	    SetTitle("Physical node " + curnode.pname + " selected");
	    repaint();
	}

	/*
	 * Jump the scroll panel so that a node is visible.
	 */
	public void MakeNodeVisible(Node thisnode) {
	    // Container.
	    ActiveMap map = (ActiveMap) thisnode.map;
	    
	    // Zap the map to it, but only if not already visible.
	    if (IsNodeVisible(thisnode))
		return;

	    // Convert point and ZapMap to it.
	    ZapMap(thisnode.scaledX(), thisnode.scaledY(), map, false);
	}

	/*
	 * Determine if a node is currently visible (in the map display)
	 */
	public boolean IsNodeVisible(Node thisnode) {
	    // Container.
	    ActiveMap map = (ActiveMap) thisnode.map;

	    System.out.println(thisnode.scaledX() + "," + thisnode.scaledY());

	    // Map coords of node in the activemap to the big panel.
	    Point P =
		javax.swing.SwingUtilities.convertPoint(map,
					thisnode.scaledX(), thisnode.scaledY(),
					PhysMapPanel);
	    // See whats showing.
	    JViewport	viewport = PhysMapScrollPane.getViewport();
	    Rectangle   R = viewport.getViewRect();

	    System.out.println(P);
	    System.out.println(R);

	    // And if the (mapped) point above is within the visible rectangle.
	    if (R.contains(P))
		return true;
	    return false;
	}

	/*
	 * Zap (scroll) a jlist so that the given index is visible.
	 */
	public void ZapList(JList jlist, int index) {
	    /*
	     * Get the bounds of what is visible, and if the index falls
	     * outside, then tell the jlist to make it visible.
	     */
	    int min = jlist.getFirstVisibleIndex();
	    int max = jlist.getLastVisibleIndex();

	    if (index < min || index > max) {
		jlist.ensureIndexIsVisible(index);
	    }
	}

	/*
	 * Zap the scroll pane so that the x,y coords are centered.
	 * Well, as centered as they can be!
	 */
	public void ZapMap(int x, int y, ActiveMap map, boolean centering) {
	    /*
	     * Must convert the event to proper coordinate system, since
	     * event comes from an ActiveMap, and we want the x,y in the
	     * space of the entire panel underlying the scrollpane, so that
	     * we can tell the viewport to go to the right place. 
	     */
	    JViewport	viewport = PhysMapScrollPane.getViewport();
	    Component	Map      = viewport.getView();

	    // Map coords in an activemap to the big panel. 
	    Point P =
		javax.swing.SwingUtilities.convertPoint(map, x, y,
							PhysMapPanel);
	    System.out.println("ZapMap: " + P.x + "," + P.y);
	    
	    /*
	     * This gives us the size of view (not the underlying panel).
	     */
	    Dimension   D = viewport.getExtentSize();

	    /*
	     * Subtract 1/2 the width/height of the visible port.
	     */
	    P.translate(- (int)(D.getWidth() / 2), - (int)(D.getHeight() / 2));

	    // But make sure we do not scroll it off screen.
	    if (P.x + D.getWidth() > PhysMapPanel.getWidth())
		P.x = (int) (PhysMapPanel.getWidth() - D.getWidth());
	    if (P.y + D.getHeight() > PhysMapPanel.getHeight())
		P.y = (int) (PhysMapPanel.getHeight() - D.getHeight());
	    if (P.x < 0)
		P.x = 0;
	    if (P.y < 0)
		P.y = 0;

	    System.out.println("ZapMap: " + P.getX() + "," + P.getY());

	    // And zap to it. Remember the point for future recenter request.
	    viewport.setViewPosition(P);
	    LastCenter = viewport.getViewRect();
	    repaint();
	}

	/*
	 * ActionListener for buttons and menu items.
	 */
 	public void actionPerformed(ActionEvent e) {
	    System.out.println("actionPerformed: " + e.getActionCommand());
	    
	    if (e.getActionCommand() == null)
		return;

	    // Look for the Menu Handler and invoke it.
	    MenuHandler	handler =
		(MenuHandler) MenuHandlers.get(e.getActionCommand());

	    if (handler != null) {
		handler.handler(e.getActionCommand());
		return;
	    }
	}

	/*
	 * The handler for both JLists (list boxes).
	 */
	class ListSelectionHandler implements ListSelectionListener {
	    public void valueChanged(ListSelectionEvent e) { 
		// If still adjusting, do nothing.
		if (e.getValueIsAdjusting())
		    return;

		ListSelectionModel lsm = (ListSelectionModel) e.getSource();
		JList		   jlist;
		
		/*
		 * Figure out which list box fired. 
		 */
		if (lsm == SelectList.getSelectionModel())
		    jlist = SelectList;
		else
		    jlist = AllPhysList;

		// And now we know the data model.
		DefaultListModel   lm = (DefaultListModel) jlist.getModel();

		/*
		 * Go through each of the items, and if the item is in
		 * the selection list, then mark it. Otherwise clear it.
		 */
		int	max     = lm.getSize();
		boolean didjump = false;

		for (int idx = 0; idx < max; idx++) {
		    Node thisnode = (Node) lm.getElementAt(idx);
		    
                    if (lsm.isSelectedIndex(idx)) {
			System.out.println("ListSelectionHandler: " +
					   "Selected index " + idx);

			thisnode.picked = true;
			// Jump only once.
			if (!didjump) {
			    MakeNodeVisible(thisnode);
			    didjump = true;
			}
		    }
		    else {
			thisnode.picked = false;
		    }
		}
		repaint();
	    }
	}

	/*
	 * Mouse listener upcall from the ActiveMap.
	 */
	public void doPopupMenu(MouseEvent e, Node node) {
	    MenuHandler handler;

	    JComponent jc = (JComponent) e.getSource();
	    jc.requestFocusInWindow();

	    /*
	     * Click (middle button) to center ...
	     */
	    if (!e.isPopupTrigger()) {
		if (e.getButton() == e.BUTTON2) 
		    ZapMap(e.getX(), e.getY(), (ActiveMap)e.getSource(), true);
		return;
	    }

	    /*
	     * Must convert the event to proper coordinate system, since
	     * event comes from an ActiveMap, and we want the x,y in the
	     * space of the entire applet, so that when the popup is
	     * generated, its where the mouse is, not 5 inches away!
	     */
	    e = javax.swing.SwingUtilities.convertMouseEvent(e.getComponent(),
							     e,
							     selector);

	    int x = e.getX();
	    int y = e.getY();
	    System.out.println("doPopupMenu: " + x + "," + y);

	    /*
	     * Otherwise show the menus. 
	     */
	    if (node == null) {
		// Toggle various menu options so not to confuse users.
		handler = (MenuHandler) MenuHandlers.get("AddNodes");
		handler.enable(! AllPhysList.isSelectionEmpty());

		handler = (MenuHandler) MenuHandlers.get("DelNodes");
		handler.enable(! SelectList.isSelectionEmpty());
		
		handler = (MenuHandler) MenuHandlers.get("NSFile");
		handler.enable(SelectList.getModel().getSize() != 0);
		
		RootPopupMenu.show(selector, x, y);
	    }
	    else {
		// So the menu handlers know what Node to mess with (later).
		PopupNode = node;

		// Toggle various menu options so not to confuse users.
		handler = (MenuHandler) MenuHandlers.get("SetVname");
		handler.enable(((PhysNode)node).selected);

		handler = (MenuHandler) MenuHandlers.get("NodeMenu_AddNodes");
		handler.enable(! AllPhysList.isSelectionEmpty());

		handler = (MenuHandler) MenuHandlers.get("NodeMenu_DelNodes");
		handler.enable(! SelectList.isSelectionEmpty());
		
		NodePopupMenu.show(selector, x, y);
	    }
	}

	/*
	 * Change the little message area text at the top of the applet.
	 */
	public void SetTitle(String message) {
	    MessageArea.setText(" ");
	    MessageArea.setText(message);
	}

	/*
	 * The handler for adding and removing nodes (menu item).
	 */
	private class AddDelNodeMenuHandler extends MenuHandler {
	    public AddDelNodeMenuHandler(JMenuItem menuitem) {
		super(menuitem);
	    }
	    public void handler(String action) {
		/*
		 * move nodes between the selected list and the all nodes list.
		 */
		JList	     fromjlist, tojlist;
		DefaultListModel frommodel, tomodel;
	    
		if (action.compareTo("AddNodes") == 0) {
		    fromjlist = AllPhysList;
		    tojlist   = SelectList;
		    frommodel = (DefaultListModel) AllPhysList.getModel();
		    tomodel   = (DefaultListModel) SelectList.getModel();
		}
		else {
		    fromjlist = SelectList;
		    tojlist   = AllPhysList;
		    frommodel = (DefaultListModel) SelectList.getModel();
		    tomodel   = (DefaultListModel) AllPhysList.getModel();
		}

		// Find out which are selected in the fromlist, and move over.
		Object	objects[] = fromjlist.getSelectedValues();

		// Do this to avoid a bunch of events getting fired!
		ClearSelection();

		for (int idx = 0; idx < objects.length; idx++) {
		    Node	thisnode = (Node) objects[idx];
		    PhysNode	pnode    = (PhysNode) thisnode;

		    frommodel.removeElement(thisnode);
		    tomodel.addElement(thisnode);
		
		    if (action.compareTo("AddNodes") == 0)
			pnode.selected  = true;
		    else {
			pnode.selected  = false;
			// clear the vname if the user decides not to use it.
			pnode.vname     = null;
		    }
		}

		/*
		 * Now we want to reorder the lists alphabetically.
		 */
		objects = frommodel.toArray();
		java.util.Arrays.sort(objects);
		frommodel.removeAllElements();

		for (int idx = 0; idx < objects.length; idx++) {
		    Node	thisnode = (Node) objects[idx];
		    
		    frommodel.add(idx, thisnode);
		    thisnode.listindex = idx;
		}

		objects = tomodel.toArray();
		java.util.Arrays.sort(objects);
		tomodel.removeAllElements();

		for (int idx = 0; idx < objects.length; idx++) {
		    Node	thisnode = (Node) objects[idx];
		    
		    tomodel.add(idx, thisnode);
		    thisnode.listindex = idx;
		}
	    }
	}

	/*
	 * A class that constructs some NS code, then "pops" up a new
	 * window with it. The window overlays the maps, cause that is
	 * the easiest thing to do.
	 */
	private class NSFileMenuHandler extends MenuHandler
	                                implements ActionListener {

	    public NSFileMenuHandler(JMenuItem menuitem) {
		super(menuitem);
	    }
	    /*
	     * Use an internal frame so that I do not have to deal with
	     * buttons and crap.
	     */
	    JInternalFrame	myframe;

	    public void handler(String action) {
		myframe = new JInternalFrame("NS File Fragment", true, true);
		myframe.setVisible(true);

		/*
		 * Use one of them edit text things.
		 */
		JTextArea NSText = new JTextArea();

		/*
		 * Get all the nodes in the list and create fragment text
		 */
		ListModel lsm = SelectList.getModel();

		for (int idx = 0; idx < lsm.getSize(); idx++) {
		    PhysNode pnode = (PhysNode) lsm.getElementAt(idx);
		    String   vname = pnode.pname;

		    if (pnode.vname != null)
			vname = pnode.vname;

		    NSText.append("set " + vname + " [$ns node]");
		    NSText.append("\n");
		    NSText.append("tb-fix-node $" + vname + " " + pnode);
		    NSText.append("\n");
		    NSText.append("\n");
		}

		/*
		 * JTextArea does not implement scrolling, so put it in
		 * a scroll window.
		 */
		JScrollPane NSScroller = new JScrollPane(NSText);

		// Add the scroller to the new frame.
		myframe.getContentPane().add(NSScroller);

		/*
		 * Okay, add ourselves to the outer layered pane, but
		 * on top of the other stuff.
		 */
		getLayeredPane().add(myframe, JLayeredPane.PALETTE_LAYER);
		myframe.setBounds(50, 50, 300, 500);
	    }
	    
	    /*
	     * ActionListener for buttons.
	     */
	    public void actionPerformed(ActionEvent e) {
	    }
	}

	/*
	 * Clear any virtual node name assigments.
	 */
	private class ClearVirtMenuHandler extends MenuHandler {
	    public ClearVirtMenuHandler(JMenuItem menuitem) {
		super(menuitem);
	    }
	    public void handler(String action) {
		Enumeration a = AllPhysNodes.elements();
	    
		while (a.hasMoreElements()) {
		    PhysNode	 pnode = (PhysNode) a.nextElement();

		    pnode.vname = null;
		}
		repaint();
	    }
	}

	/*
	 * Enter a vnode name to a popup window.
	 */
	private class SetVnameMenuHandler extends MenuHandler {
	    public SetVnameMenuHandler(JMenuItem menuitem) {
		super(menuitem);
	    }
	    public void handler(String action) {
		PhysNode pnode = (PhysNode) PopupNode;

		String str = (String)
		    JOptionPane.showInputDialog(selector,
				"Enter name for " + pnode.pname,
				(pnode.selected ? pnode.vname : null));
		
		str = str.trim();
		System.out.println("New Vname: " + str);
		if (str.compareTo("") == 0)
		    pnode.vname = null;
		else
		    pnode.vname = str;
		repaint();
	    }
	}

	/*
	 * Handle Zooming
	 */
	private class ZoomMenuHandler extends MenuHandler {
	    public ZoomMenuHandler(JMenuItem menuitem) {
		super(menuitem);
	    }
	    public void handler(String action) {
		// Remember where the viewport is and its size.
		JViewport	viewport = PhysMapScrollPane.getViewport();
		Rectangle       R1 = viewport.getViewRect();
		double		factor;
		
		if (action.compareTo("Reset") == 0)
		    factor = 1.0 / scale;
		else if (action.compareTo("ZoomIn") == 0)
		    factor = 1.5;
		else
		    factor = 0.5;

		scale = scale * factor;
		
		for (int index = 0; index < PhysMaps.size(); index++) {
		    ActiveMap map = (ActiveMap) PhysMaps.elementAt(index);

		    map.rescale();
		}
		// Force everything to layout again.
		revalidate();
		validate();

		// Tell rulers to recompute their size.
		Ruler ruler =
		    (Ruler) PhysMapScrollPane.getColumnHeader().getView();
		ruler.reset();
		
		ruler =
		    (Ruler) PhysMapScrollPane.getRowHeader().getView();
		ruler.reset();

		// Force everything to layout again.
		revalidate();
		validate();
		    
		/*
		 * Okay scale P so that it moves (thereby keeping the
		 * original position more or less centered). 
		 */
		// Center point before.
		Point		P1 = new Point(R1.x + R1.width/2,
					       R1.y + R1.height/2);
		// Center point after scaling.
		Point		P2 = new Point((int) (P1.x * factor),
					       (int) (P1.y * factor));

		System.out.println(P1 + "," + P2);

		
		// Need a rectangle for scrollrect
		Rectangle R2 = new Rectangle(P2.x - R1.width/2,
					     P2.y - R1.height/2,
					     R1.width, R1.height);
		System.out.println(R2);

		// Update lastcenter by the scale.
		if (LastCenter != null) {
		    LastCenter.x = (int) (LastCenter.x * factor);
		    LastCenter.y = (int) (LastCenter.y * factor);
		}

		if (action.compareTo("Reset") == 0 && LastCenter != null)
		    PhysMapPanel.scrollRectToVisible(LastCenter);
		else
		    PhysMapPanel.scrollRectToVisible(R2);
		
		repaint();
	    }
	}

	private class RecenterMenuHandler extends MenuHandler {
	    public RecenterMenuHandler(JMenuItem menuitem) {
		super(menuitem);
	    }
	    public void handler(String action) {
		JViewport	viewport = PhysMapScrollPane.getViewport();

		if (LastCenter == null)
		    return;

		System.out.println(LastCenter);

		PhysMapPanel.scrollRectToVisible(LastCenter);
		repaint();
	    }
	}

	/*
	 * This class and code comes from one of the examples on the Sun
	 * Java site. 
	 */
	public class Ruler extends JComponent {
	    private int SIZE       = 30;

	    private int orientation;
	    private int increment;
	    private int units;

	    public Ruler(int or) {
		orientation = or;
		// Tick every meter at any scale.
		increment = (int) (pixels_per_meter * scale);
		units = increment * 5;
	    }
	    public void reset () {
		setsizes();
		// Tick every meter at any scale.
		increment = (int) (pixels_per_meter * scale);
		units = increment * 5;
	    }
	    private void setsizes() {
		int		width;
		int		height;

		if (orientation == HORIZONTAL) {
		    width  = PhysMapPanel.getWidth();
		    height = SIZE;
		}
		else {
		    width  = SIZE;
		    height = PhysMapPanel.getHeight();
		}
		System.out.println(orientation + ": " + width + "," + height);
		setPreferredSize(new Dimension(width, height));
	    }

	    public int getIncrement() {
		return increment;
	    }

	    public void setPreferredHeight(int ph) {
		setPreferredSize(new Dimension(SIZE, ph));
	    }
	    
	    public void setPreferredWidth(int pw) {
		setPreferredSize(new Dimension(pw, SIZE));
	    }
	    
	    protected void paintComponent(Graphics g) {
		Rectangle drawHere = g.getClipBounds();
		//System.out.println(drawHere);

		// Fill clipping area with dirty brown/orange.
		g.setColor(new Color(230, 163, 4));
		g.fillRect(drawHere.x, drawHere.y,
			   drawHere.width, drawHere.height);

		// Do the ruler labels in a small font that's black.
		g.setFont(RulerFont);
		g.setColor(Color.black);

		// Some vars we need.
		int end        = 0;
		int start      = 0;
		int tickLength = 0;
		String label   = null;

		// Use clipping bounds to calculate first and last locations.
		if (orientation == HORIZONTAL) {
		    start = (drawHere.x / increment) * increment;
		    end = (((drawHere.x + drawHere.width) / increment) + 1)
			* increment;
		}
		else {
		    start = (drawHere.y / increment) * increment;
		    end = (((drawHere.y + drawHere.height) / increment) + 1)
			* increment;
		}

		//System.out.println("Rulers Painting(" + orientation + "): " +
		//		       start + "," + end);

		// Draw ticks
		for (int i = start; i < end; i += increment) {
		    if (i != 0 && i % units == 0)  {
			tickLength = 10;
			label = Integer.toString(i/increment);
		    }
		    else {
			tickLength = 7;
			label = null;
		    }
		    
		    if (tickLength != 0) {
			if (orientation == HORIZONTAL) {
			    g.drawLine(i, SIZE-1, i, SIZE-tickLength-1);
			    if (label != null)
				g.drawString(label, i-4, 15);
			}
			else {
			    g.drawLine(SIZE-1, i, SIZE-tickLength-1, i);
			    if (label != null)
				g.drawString(label, 2, i+5);
			}
		    }
		}
	    }
	}
    }

    /*
     * Get the virtnode info and return a dictionary to the caller.
     */
    public Dictionary GetVirtInfo() {
	Dictionary	virtnodes = new Hashtable();
	String		str;
	int		index = 0;
	int		min_x = 999999, min_y = 999999;
	int		max_x = 0, max_y = 0;

	// Fake up one.
	if (shelled) {
	    String	vname  = "nodeA";
	    VirtNode	vnode  = new VirtNode();

	    vnode.index = index++;
	    vnode.vname = vname;
	    vnode.fixed = "pc1";
	    virtnodes.put(vname, vnode);

	    return virtnodes;
	}

	BufferedReader  myreader  = OpenURL("virtinfo.php3", null);
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

		if (fixed.length() == 0) {
		    fixed = null;
		}
		vnode.index = index++;
		vnode.vname = vname;
		vnode.fixed = fixed;
		vnode.pname = fixed;
		virtnodes.put(vname, vnode);
	    }
	    myreader.close();
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
    int foo = 0;
    public Dictionary GetNodeInfo(String building, String floor) {
	Dictionary	physnodes = new Hashtable();
	String		str;
	int		index = 0;

	// Fake up one.
	if (shelled) {
	    String	pname  = "pc" + foo;
	    foo++;
	    PhysNode	pnode  = new PhysNode();

	    pnode.index = index++;
	    pnode.pname = pname;
	    pnode.type  = "pc";
	    pnode.mobile= false;
	    pnode.allocated = false;
	    pnode.setX(100);
	    pnode.setY(100);
	    physnodes.put(pname, pnode);

	    return physnodes;
	}

	BufferedReader  myreader  = OpenURL("nodeinfo.php3",
					    "selector=1" +
					    "&floor=" + floor +
					    "&building=" + building);
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
		pnode.setX(Integer.parseInt(tokens.nextToken().trim()));
		pnode.setY(Integer.parseInt(tokens.nextToken().trim()));
		
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
	    + "&nocookieuid="
	    + URLEncoder.encode(uid)
	    + "&nocookieauth="
	    + URLEncoder.encode(auth);

	if (pid != null && eid != null) {
	    urlstring = urlstring
		+ "&pid=" + pid
		+ "&eid=" + eid;
	}

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
	int myWidth  = 1200;
        final NodeSelect selector = new NodeSelect();
	
	try
	{
	    URL url = new URL("file:///tmp/robots-4.jpg");

	    selector.pid      = "testbed";
	    selector.eid      = "one-node";
	    selector.myWidth  = myWidth;
	    selector.myHeight = myHeight;
	    selector.init(true, url);
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
