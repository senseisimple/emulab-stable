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
    
    /* Constants for virt image link */
    int ZOOM   = 3;
    int DETAIL = 2;

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
	URL		expURL = null;
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
	selector = new Selector(null, phyURL);
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
	public String toString() {
	    return this.pname;
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
    }

    /*
     * A utility class for menu items. Associate a handler with each menu
     * item, and store those in a dictionary so we can find them when the
     * mousehandler fires.
     */
    private class MenuHandler {
	public void handler(String action) {
	    MyDialog("MenuHandler", "No handler specified!");
	}
    }

    /*
     * The actual object.
     */
    private class Selector extends JPanel implements MouseListener,
						     ActionListener {
	JLabel		MessageArea;
	// Indexed by the virtual name, points to VirtNode struct above.
	Dictionary	VirtNodes;
	// A list of the ActiveMaps.
	Vector		PhysMaps = new Vector(10, 10);
	// For the menu items.
	Dictionary	MenuHandlers = new Hashtable();

	// Not sure how I am going to use this yet. Zooming eventually ...
	double		scale = 1.0;

	// The lists boxes, one for all nodes and one for selected nodes.
	JList		AllPhysList;
	JList           SelectList;

	// The physmap scroll pane.
	JScrollPane	PhysMapScrollPane;

	// The root menu.
	JPopupMenu	RootPopupMenu;

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
		return iconHeight;
	    }
	    public int getIconWidth() {
		return iconWidth;
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
		    Node	node = (Node)e.nextElement();
		    String      label;

		    if (node instanceof VirtNode) {
			label = node.vname;
		    }
		    else {
			label = node.pname;
		    }

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
	    int		width, height, xoff, yoff;
	    boolean     isvirtmap;
	    Dictionary  NodeList;
	    
	    public ActiveMap(ImageIcon icon, Dictionary nodelist,
			     int thiswidth, int thisheight, boolean virtmap) {
		super(new FloorIcon(icon, nodelist));
		this.addMouseListener(this);

		NodeList  = nodelist;
		isvirtmap = virtmap;
		width  = icon.getIconWidth();
		height = icon.getIconHeight();

		// Set the back pointer.
		Enumeration e = nodelist.elements();
		while (e.hasMoreElements()) {
		    Node node = (Node) e.nextElement();

		    node.map = this;
		}

		if (width < thiswidth)
		    xoff = -((thiswidth / 2) - (width / 2));
		if (height < (thisheight / 2))
		    yoff = ((thisheight / 2) / 2) - (height / 2);
	    }
	    public void mousePressed(MouseEvent e) {
		int button   = e.getButton();
		int modifier = e.getModifiersEx();
		int x      = e.getX() + xoff;
		int y      = e.getY() - yoff;

		System.out.println("Clicked on " + e.getX() + "," + e.getY());
		System.out.println("Clicked on " + x + "," + y);

		if (x > 0 && y > 0 && x <= width && y <= height)
		    FindNode(this, x, y, modifier);
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

        public Selector(URL expURL, URL phyURL) {
	    ActiveMap   activemap;
	    Dictionary  PhysNodes;
	    
	    /*
	     * We leave room on the right for the two list boxes.
	     * The height depends on whether there is a virtual map box.
	     */
	    int mapwidth  = myWidth - LISTBOX_WIDTH;
	    int mapheight = myHeight;
	    if (expURL != null)
		mapheight = mapheight / 2;

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
	    AllPhysList.getSelectionModel().addListSelectionListener(
			new AllPhysListSelectionHandler());
	    SelectList.getSelectionModel().addListSelectionListener(
			new SelectListSelectionHandler());
 
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
	     * Get the virtnodes info, but this part is optional. 
	     */
	    if (expURL != null) {
		if ((VirtNodes = GetVirtInfo()) == null) {
		    MyDialog("GetVirtInfo",
			     "Failed to get virtnode list from server");
		}
		activemap = new ActiveMap(getImageIcon(expURL), VirtNodes,
					  mapwidth, mapheight, true);

		/*
		 * The upper scrollpane holds the virtual experiment image.
		 */
		JScrollPane scrollPane = new JScrollPane(activemap);
		scrollPane.setPreferredSize(new Dimension(mapwidth,
							  mapheight));
		MapPanel.add(scrollPane);
	    }

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

		/*
		 * Add each physnode to the the listbox of phys nodes.
		 */
		DefaultListModel model =
		    (DefaultListModel) AllPhysList.getModel();

		Enumeration e = PhysNodes.elements();
		while (e.hasMoreElements()) {
		    Node pnode = (PhysNode) e.nextElement();
		    model.addElement(pnode);
		    // Get back its index so we know where it landed.
		    pnode.listindex = model.indexOf(pnode);
		}

		/*
		 * Stick in a title to let the user know what floor/building.
		 */
		JLabel TitleArea =
		    new JLabel("Here is where the building/floor name goes");
		TitleArea.setAlignmentX(Component.CENTER_ALIGNMENT);
		ScrolledPanel.add(TitleArea);

		activemap = new ActiveMap(getImageIcon(phyURL), PhysNodes,
					  mapwidth, mapheight, false);
		activemap.setAlignmentX(Component.CENTER_ALIGNMENT);
		ScrolledPanel.add(activemap);

		// So popup trigger is seen over maps too.
		activemap.addMouseListener(this);

		// Remember them all in a list.
		PhysMaps.insertElementAt(activemap, i);
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

	    menuitem = new JMenuItem("Add Nodes");
	    menuitem.setActionCommand("AddNodes");
	    menuitem.addActionListener(this);
	    MenuHandlers.put("AddNodes", new AddDelNodeMenuHandler());
	    RootPopupMenu.add(menuitem);

	    menuitem = new JMenuItem("Remove Nodes");
	    menuitem.setActionCommand("DelNodes");
	    menuitem.addActionListener(this);
	    MenuHandlers.put("DelNodes", new AddDelNodeMenuHandler());
	    RootPopupMenu.add(menuitem);

	    menuitem = new JMenuItem("Create NS File");
	    menuitem.setActionCommand("NSFile");
	    menuitem.addActionListener(this);
	    MenuHandlers.put("NSFile", new NSFileMenuHandler());
	    RootPopupMenu.add(menuitem);
	    
	    menuitem = new JMenuItem("Close This Menu");
	    RootPopupMenu.add(menuitem);
	    RootPopupMenu.setBorder(
			BorderFactory.createLineBorder(Color.black));
	    this.addMouseListener(this);	    
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
	 * Made a selection on the map. Figure out what it was.
	 */
	public void FindNode(ActiveMap Map, int x, int y, int modifier) {
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
	    if (curnode == null)
		return;

	    // Clear current selection if no modifier.
	    if ((modifier &
		 (InputEvent.SHIFT_DOWN_MASK|InputEvent.CTRL_DOWN_MASK))
		== 0) {
		ClearSelection();
	    }
	    curnode.picked = true;
	    
	    /*
	     * Physical or virtual node?
	     */
	    if (! Map.isvirtmap) {
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

	    }
	    // Say something in the message area at the top.
	    if (Map.isvirtmap) {
		SetTitle("Virtual node " + curnode.vname + " selected");
	    }
	    else {
		SetTitle("Physical node " + curnode.pname + " selected");
	    }
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
	 * Event listeners for the listboxes. We use one for each of the
	 * boxes
	 */
	class AllPhysListSelectionHandler implements ListSelectionListener {
	    public void valueChanged(ListSelectionEvent e) { 
		// If still adjusting, do nothing.
		if (e.getValueIsAdjusting())
		    return;

		ListSelectionModel lsm = (ListSelectionModel) e.getSource();
		DefaultListModel   lm  =
		    (DefaultListModel) AllPhysList.getModel();

		/*
		 * Go through each of the items, and if the item is in
		 * the selection list, then mark it. Otherwise clear it.
		 */
		int max = lm.getSize();

		for (int idx = 0; idx < max; idx++) {
		    Node thisnode = (Node) lm.getElementAt(idx);
		    
                    if (lsm.isSelectedIndex(idx)) {
			System.out.println("AllPhysListSelectionHandler: " +
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
	class SelectListSelectionHandler implements ListSelectionListener {
	    public void valueChanged(ListSelectionEvent e) { 
		// If still adjusting, do nothing.
		if (e.getValueIsAdjusting())
		    return;

		ListSelectionModel lsm = (ListSelectionModel) e.getSource();
		DefaultListModel   lm  =
		    (DefaultListModel) SelectList.getModel();

		/*
		 * Go through each of the items, and if the item is in
		 * the selection list, then mark it. Otherwise clear it.
		 */
		int max = lm.getSize();

		for (int idx = 0; idx < max; idx++) {
		    Node thisnode = (Node) lm.getElementAt(idx);
		    
                    if (lsm.isSelectedIndex(idx)) {
			System.out.println("SelectListSelectionHandler: " +
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
	 * Mouse listener stuff.
	 */
	public void mousePressed(MouseEvent e) {
	    System.out.println("Clicked(B) on " + e.getX() + "," + e.getY());
	    
	    if (e.isPopupTrigger()) {
		RootPopupMenu.show(selector, e.getX(), e.getY());
	    }
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
		    else
			pnode.selected  = false;
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

		    NSText.append("set " + pnode + " [$ns node]");
		    NSText.append("\n");
		    NSText.append("tb-fix-node $" + pnode + " " + pnode);
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
	    vnode.fixed = null;
	    vnode.setX(100);
	    vnode.setY(100);
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
		int    vis_x = Integer.parseInt(tokens.nextToken().trim());
		int    vis_y = Integer.parseInt(tokens.nextToken().trim());

		if (fixed.length() == 0) {
		    fixed = null;
		}
		vnode.index = index++;
		vnode.vname = vname;
		vnode.fixed = fixed;
		vnode.pname = fixed;
		vnode.setX(vis_x);
		vnode.setY(vis_y);

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

		vnode.x = ((vnode.x - min_x) * ZOOM) + X_ADJUST;
		vnode.y = ((vnode.y - min_y) * ZOOM) + Y_ADJUST;

		System.out.println(vnode.vname + " " + vnode.x + "," + vnode.y);
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
