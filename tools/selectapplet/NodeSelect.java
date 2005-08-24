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
    String			uid, auth, pid, eid, floor, building;
    double			pixels_per_meter = 1.0;
    URL				urlServer;
    static final DecimalFormat  FORMATTER = new DecimalFormat("0.00");
    
    /* From the floormap generation code. */
    int X_ADJUST = 60;
    int Y_ADJUST = 60;

    // The font width/height adjustment, for drawing labels.
    int FONT_HEIGHT = 14;
    int FONT_WIDTH  = 6;
    Font OurFont    = null;

    // The width of the list boxes.
    int LISTBOX_WIDTH = 150;

    public void init() {
	URL		phyURL = null;
	String		phyurl;

	OurFont = new Font("Arial", Font.PLAIN, 14);
	
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
	    
	    // form the URL that we use to get the physical layout icon
	    phyURL = new URL(this.getCodeBase(),
			     phyurl
			     + "&nocookieuid="
			     + URLEncoder.encode(uid)
			     + "&nocookieauth="
			     + URLEncoder.encode(auth));
	}
	catch(Throwable th)
	{
	    th.printStackTrace();
	}
	init(false, phyURL);
    }

    public void start() {
    }
  
    public void stop() {
    }

    // For the benefit of running from the shell for testing.
    public void init(boolean fromshell, URL phyURL) {
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
	selector = new Selector(phyURL);
        MyJPanel.add(selector);

	/*
	 * Add the JPanel to the layeredPane, but give its size since
	 * there is no layout manager.
	 */
	MyLPane.add(MyJPanel, JLayeredPane.DEFAULT_LAYER);
	MyJPanel.setBounds(0, 0, myWidth, myHeight);
    }

    /*
     * A container for stuff that is common to both virtnodes and physnodes.
     */
    private class Node {
	String		vname;
	String		pname;
	int		radius = 15;	// Radius of circle, in pixels
	int		size   = 20;	// Actual size, in pixels
        int		x, y;		// Current x,y coords in pixels
	int		listindex;	// Where it sits in its listbox.
	boolean		picked;		// Node is within a selection
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
    private class Selector extends JPanel implements ActionListener {
	JLabel		MessageArea;
	// Indexed by the virtual name, points to VirtNode struct above.
	Dictionary	VirtNodes;
	// Indexed by the physical name, points to PhysNode struct above.
	Dictionary	AllPhysNodes = new Hashtable();
	// A list of the ActiveMaps.
	Vector		PhysMaps = new Vector(10, 10);
	// For the menu items.
	Dictionary	MenuHandlers = new Hashtable();

	// Not sure how I am going to use this yet. Zooming eventually ...
	double		scale = 0.5;

	// The lists boxes, one for all nodes and one for selected nodes.
	JList		AllPhysList;
	JList           SelectList;

	// The physmap scroll pane.
	JScrollPane	PhysMapScrollPane;
	// The panel behind the scroll pane.

	// The root menu.
	JPopupMenu	RootPopupMenu;
	// The node menu.
	JPopupMenu	NodePopupMenu;
	// For the node menu, need to know what the node was!
	Node		PopupNode;

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

		iconWidth  = icon.getIconWidth();
		iconHeight = icon.getIconHeight();
	    }
	    public int getIconHeight() {
		return (int) (iconHeight * scale);
	    }
	    public int getIconWidth() {
		return (int) (iconWidth * scale);
	    }

	    /*
	     * The paint method sticks the little dots in.
	     */
	    public void paintIcon(Component c, Graphics g, int x, int y) {
		System.out.println("Icon Painting at " + x + "," + y);

		Graphics2D g2 = (Graphics2D) g;
		g2.scale(scale, scale);
		myicon.paintIcon(c, g2, x, y);

		/*
		 * Draw all the nodes on this map.
		 */
		Enumeration e = NodeList.elements();

		while (e.hasMoreElements()) {
		    Node	node  = (Node)e.nextElement();
		    String      label = node.pname;

		    /*
		     * Draw a little circle where the node lives.
		     */
		    if (node.picked)
			g.setColor(Color.green);
		    else
			g.setColor(Color.blue);
		    
		    g.fillOval(node.x - node.radius,
			       node.y - node.radius,
			       node.radius * 2,
			       node.radius * 2);

		    /*
		     * Draw a label below the dot.
		     */
		    int lx  = node.x - ((FONT_WIDTH * label.length()) / 2);
		    int ly  = node.y + node.radius + FONT_HEIGHT;

		    g.setColor(Color.black);
		    g.drawString(label, lx, ly);
		}
		// Restore Graphics object
		g2.scale(1.0 - scale, 1.0 - scale);
	    }
	}	    

	/*
	 * A little class that creates an active map for clicking on.
	 */
	private class ActiveMap extends JLabel implements MouseListener {
	    int		width, height, xoff = 0, yoff = 0;
	    boolean     isvirtmap;
	    Dictionary  NodeList;
	    FloorIcon   flooricon;
	    
	    public ActiveMap(FloorIcon flooricon, Dictionary nodelist,
			     int thiswidth, int thisheight, boolean virtmap) {
		super(flooricon);
		this.flooricon = flooricon;
		this.addMouseListener(this);

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

	    public void rescale() {
		// Pick up new scale.
		width  = flooricon.getIconWidth();
		height = flooricon.getIconHeight();

		this.setPreferredSize(new Dimension(width, height));
		this.revalidate();
	    }
	    public void mousePressed(MouseEvent e) {
		int button   = e.getButton();
		int modifier = e.getModifiersEx();
		int x        = e.getX() + xoff;
		int y        = e.getY() - yoff;
		Node node    = null;

		System.out.println("Clicked on " + e.getX() + "," + e.getY());
		System.out.println("Clicked on " + x + "," + y);

		if (x > 0 && y > 0 && x <= width && y <= height)
		    node = FindNode(this, (int)(x / scale), (int)(y / scale));
		
		if (!e.isPopupTrigger()) {
		    if (node != null) {
			SelectNode(node, modifier);
			return;
		    }
		}
		// Forward to outer event handler, including the node
		selector.doPopupMenu(e, node);
	    }
	    public void mouseReleased(MouseEvent e) {
	    }	

	    public void mouseEntered(MouseEvent e) {
	    }

	    public void mouseExited(MouseEvent e) {
	    }

	    public void mouseClicked(MouseEvent e) {
	    }
	}

        public Selector(URL phyURL) {
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
	    ListPanel.add(new JLabel("Selected Nodes"));

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
	    JPanel  ScrolledPanel = new JPanel(true);
	    
	    /*
	     * Specify vertical placement of components inside this panel
	     */
	    ScrolledPanel.setLayout(new BoxLayout(ScrolledPanel,
						  BoxLayout.Y_AXIS));

	    /*
	     * Get the physnodes info. Each submap is added to a large
	     * panel, which we then add to a scroll pane.
	     */
	    for (int i = 0; i < 3; i++) {
		if ((PhysNodes = GetNodeInfo()) == null) {
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
		    new JLabel("Here is where the building/floor name goes");
		TitleArea.setAlignmentX(Component.CENTER_ALIGNMENT);
		ScrolledPanel.add(TitleArea);

		activemap = new
		    ActiveMap(new FloorIcon(getImageIcon(phyURL), PhysNodes),
			      PhysNodes, mapwidth, mapheight, false);
		activemap.setAlignmentX(Component.CENTER_ALIGNMENT);
		ScrolledPanel.add(activemap);

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

		if (pnode.selected)
		    model = (DefaultListModel) SelectList.getModel();
		else
		    model = (DefaultListModel) AllPhysList.getModel();
		
		model.addElement(pnode);
		// Get back its index so we know where it landed.
		pnode.listindex = model.indexOf(pnode);
	    }

	    /*
	     * Create a surrounding scroll pane.
	     */
	    PhysMapScrollPane = new JScrollPane(ScrolledPanel);
	    PhysMapScrollPane.setPreferredSize(new Dimension(mapwidth,
							     mapheight));
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
	    
	    menuitem = new JMenuItem("Close This Menu");
	    NodePopupMenu.add(menuitem);
	    NodePopupMenu.setBorder(
			BorderFactory.createLineBorder(Color.black));
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
	    }
	    else {
		AllPhysList.addSelectionInterval(pnode.listindex,
						 pnode.listindex);
	    }

	    // Say something in the message area at the top.
	    SetTitle("Physical node " + curnode.pname + " selected");
	    repaint();
	}

	/*
	 * Select a specific node and jump the scroll box to it.
	 */
	public void PickNode(Node thisnode) {
	    // Container.
	    ActiveMap map = (ActiveMap) thisnode.map;
	    
	    thisnode.picked = true;

	    // Zap the map to it.
            map.scrollRectToVisible(new Rectangle(thisnode.x - 100,
						  thisnode.y - 100,
						  200, 200));
	}

	/*
	 * Zap the scroll pane so that the x,y coords are centered.
	 * Well, as centered as they can be!
	 */
	public void ZapTo(MouseEvent e) {
	    /*
	     * Must convert the event to proper coordinate system, since
	     * event comes from an ActiveMap, and we want the x,y in the
	     * space of the entire panel underlying the scrollpane, so that
	     * we can tell the viewport to go to the right place. 
	     */
	    JViewport	viewport = PhysMapScrollPane.getViewport();
	    Component	Map      = viewport.getView();
	    
	    e = javax.swing.SwingUtilities.convertMouseEvent(e.getComponent(),
							     e, Map);

	    int x = e.getX();
	    int y = e.getY();
	    System.out.println("Zapto: " + x + "," + y);
	    
	    /*
	     * Form a new Point. Start with the point we want to zap to.
	     */
	    Point	P = new Point(x, y);

	    /*
	     * This gives us the size of view (not the underlying panel).
	     */
	    Dimension   D = viewport.getExtentSize();

	    /*
	     * Subtract 1/2 the width/height of the visible port.
	     */
	    P.translate(- (int)(D.getWidth() / 2), - (int)(D.getHeight() / 2));

	    System.out.println("Zapto: " + D.getWidth() + "," + D.getHeight());

	    /*
	     * Form a rectangle that is centered around the point.
	     * Sheesh, you would think there would be a builtin for this!
	     */
	    System.out.println("Zapto: " + P.getX() + "," + P.getY());

	    viewport.setViewPosition(P);
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
		int max = lm.getSize();

		for (int idx = 0; idx < max; idx++) {
		    PhysNode thisnode = (PhysNode) lm.getElementAt(idx);
		    
                    if (lsm.isSelectedIndex(idx)) {
			System.out.println("ListSelectionHandler: " +
					   "Selected index " + idx);

			PickNode(thisnode);
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

	    /*
	     * Click to center ...
	     */
	    if (!e.isPopupTrigger()) {
		ZapTo(e);
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

		for (int idx = 0; idx < objects.length; idx++) {
		    Node	thisnode = (Node) objects[idx];
		    PhysNode	pnode    = (PhysNode) thisnode;

		    frommodel.removeElement(thisnode);
		    tomodel.addElement(thisnode);
		
		    // Get back its index so we know where it landed.
		    thisnode.listindex = tomodel.indexOf(thisnode);

		    if (action.compareTo("AddNodes") == 0)
			pnode.selected  = true;
		    else {
			pnode.selected  = false;
			// clear the vname if the user decides not to use it.
			pnode.vname     = null;
		    }
		}
		ClearSelection();
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
		if (action.compareTo("ZoomIn") == 0)
		    scale = scale * 1.2;
		else
		    scale = scale * 0.8;

		for (int index = 0; index < PhysMaps.size(); index++) {
		    ActiveMap map = (ActiveMap) PhysMaps.elementAt(index);

		    map.rescale();
		}
		repaint();
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
    public Dictionary GetNodeInfo() {
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
