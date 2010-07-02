/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006-2007 University of Utah and the Flux Group.
 * All rights reserved.
 */

import javax.swing.*;
import javax.swing.event.*;
import java.awt.*;
import java.awt.event.*;
import java.awt.image.*;
import java.util.*;

public class NodeMapPanel extends javax.swing.JPanel 
    implements ChangeListener,ImageObserver,DatasetModelListener {
    
    private Image bgImage;
    // the widgets vector is what actually gets drawn, but the elements
    // in the following hashtables are provided for ease of ref
    private Hashtable nodeWidgets;
    private boolean selectEnabled;
    private Hashtable linkWidgets;
    private Vector widgets;
    private MapDataModel model;
    private ControlPanel controlPanel;
    private float scaleFactor;
    private JScrollPane containingSP;
    
    private Point origPress;
    private Point origViewPosition;
    private int lastShiftX;
    private int lastShiftY;
    
    private Point coordToCenter;
    private BufferedImage bufImage;
    
    // our connection to sanity
    private DatasetModel dmodel;
    
    // Bean-ify.
    public NodeMapPanel() {
        super();
    }
    
    public NodeMapPanel(DatasetModel dm) {
        super();
        
        this.dmodel = dm;
        
        this.widgets = new Vector();
        this.bgImage = null;
        this.nodeWidgets = new Hashtable();
        this.linkWidgets = new Hashtable();
        this.model = null;
        this.selectEnabled = false;
        
        setPreferredSize(new Dimension(800,600));
        
        coordToCenter = new Point(0,0);
        
        initComponents();
//        
//        this.addMouseListener(new MouseListener() {
//            public void mousePressed(MouseEvent e) {
//               System.err.println("Mouse pressed; # of clicks: "
//                    + e.getClickCount());
//            }
//
//            public void mouseReleased(MouseEvent e) {
//               System.err.println("Mouse released; # of clicks: "
//                            + e.getClickCount());
//            }
//
//            public void mouseEntered(MouseEvent e) {
//               System.err.println("Mouse entered");
//            }
//
//            public void mouseExited(MouseEvent e) {
//               System.out.println("Mouse exited");
//            }
//
//            public void mouseClicked(MouseEvent e) {
//               System.err.println("Mouse clicked (# of clicks: "
//                            + e.getClickCount());
//            }
//        });
        
        //setOpaque(true);
        
        // add our custom mouse listener here:
        this.addMouseListener(new EnhancedMouseListener() {
            public void click(MouseEvent evt) {
                handleSingleClick(evt);
            }
            
            public void doubleClick(MouseEvent evt) {
                handleDoubleClick(evt);
            }
            
            public void press(MouseEvent evt) {
                handlePress(evt);
            }
        });
        
        this.addMouseMotionListener(new MouseMotionAdapter() {
            public void mouseDragged(MouseEvent evt) {
                handleDrag(evt);
            }
        });
        
        this.dmodel.addDatasetModelListener(this);
        
        // see if we have a current model...
        if (this.dmodel.getCurrentModel() != null) {
            setModel(this.dmodel.getCurrentModel());
            this.model.addChangeListener(this);
        }
        
    }
    
    public void newDataset(String dataset,DatasetModel model) { return; }
    
    public void datasetUnloaded(String dataset,DatasetModel model) { return; }
    
    public void currentDatasetChanged(String dataset,DatasetModel model) {
        this.setModel(model.getCurrentModel());
    }
    
    public void setContainingScrollPane(JScrollPane jsp) {
        this.containingSP = jsp;
        
        this.containingSP.getVerticalScrollBar().setUnitIncrement(10);
        this.containingSP.getHorizontalScrollBar().setUnitIncrement(10);
    }
    
    private void handleDrag(MouseEvent evt) {
        //System.out.println("evt=("+evt.getX()+","+evt.getY()+")");
        Point evtShifted = new Point(evt.getX() - lastShiftX,evt.getY() - lastShiftY);
        
        JViewport jvp = this.containingSP.getViewport();
        Point cvp = jvp.getViewPosition();
        
        int dx = (int)evtShifted.getX() - this.origPress.x;
        int dy = (int)evtShifted.getY() - this.origPress.y;
        //Point nvp = new Point(origViewPosition.x + dx,origViewPosition.y + dy);
        Point nvp = new Point(origViewPosition.x - dx,origViewPosition.y - dy);
        
        //System.out.println("nvp_b_c="+nvp);
        
        if (nvp.x < 0) {
            nvp.x = 0;
        }
        else if ((nvp.x + jvp.getExtentSize().getWidth()) > this.getWidth()) {
            nvp.x = (int)(this.getWidth() - jvp.getExtentSize().getWidth());
        }
        if (nvp.y < 0) {
            nvp.y = 0;
        }
        else if ((nvp.y + jvp.getExtentSize().getHeight()) > this.getHeight()) {
            nvp.y = (int)(this.getHeight() - jvp.getExtentSize().getHeight());
        }
        
        //System.out.println("nvp="+nvp+",cvp="+cvp);
        
        lastShiftX = nvp.x - origViewPosition.x;
        lastShiftY = nvp.y - origViewPosition.y;
        
        // set new center coord based on the new viewport position:
        Dimension jd = jvp.getExtentSize();
        this.coordToCenter = new Point((nvp.x+jd.width)/2,(nvp.y+jd.height)/2);
        //this.coordToCenter = new Point((int)(evt.getX()/this.scaleFactor),(int)(evt.getY()/scaleFactor));
        
        jvp.setViewPosition(nvp);
        jvp.invalidate();
        
    }
    
    public void handlePress(MouseEvent evt) {
        this.origPress = evt.getPoint();
        this.origViewPosition = this.containingSP.getViewport().getViewPosition();
        System.out.println("origPress="+origPress);
        
        this.lastShiftX = 0;
        this.lastShiftY = 0;
    }
    
    private void setModel(MapDataModel model) {
        if (this.model != null) {
            // remove ourselves as a listener, just in case.
            this.model.removeChangeListener(this);
        }
        
        this.model = model;
        
        this.model.addChangeListener(this);
    }
    
    public boolean updateImage(Image img,int infoflags,
                               int x,int y,int width,int height) {
        if (width > 1 && height > 1) {
            System.out.println("NMP: w = " + width + ",h = " + height);
            this.repaint();
            return false;
        }
        return true;
    }
    
    public void stateChanged(ChangeEvent e) {
        // here, we'll have to read the whole state of the data model, 
        // and figure out what to draw.  Probably best if the model
        // has some functions to tell us which nodes and links to draw.
        
        // if e == this, assume that we only we need to redraw the bgImage.
        
        System.out.println("NMP: stateChanged event");
        
        // check the model selection mode and update ourself:
        this.selectEnabled = this.model.isModeSelect();
        Vector selectedNodes = this.model.getSelection();
        
        if (e.getSource() != this) {
            this.scaleFactor = this.model.getScaleFactor();

            this.widgets.clear();
            this.nodeWidgets.clear();
            this.linkWidgets.clear();

            String[] nodes = model.getNodes();
            if (nodes != null) {
                for (int i = 0; i < nodes.length; ++i) {
                    //System.out.println("nodes["+i+"] = '"+nodes[i]+"'");
                    Point posit = model.getPosition(nodes[i]);
                    NodeWidget nw = new NodeWidget(nodes[i],
                                                   (int)(posit.x*this.scaleFactor),
                                                   (int)(posit.y*this.scaleFactor));
                    this.widgets.add(nw);
                    this.nodeWidgets.put(nodes[i],nw);
                }
            }

            // now grab the links...
            GenericLinkStats links[] = model.getCurrentLinks();
            String property = model.getCurrentProperty();
            Float rangeDelta = model.getCurrentPropertyDelta();
            if (links != null) { // && false) {
                for (int i = 0; i < links.length; ++i) {
                    // here's how we're gonna handle this:
                    // each key in the linkWidgets hash is "nodeA,nodeB",
                    // with values of LinkWidgets.
                    // if the current link is "A,B", and the hash already
                    // has "B,A", we'll make "B,A" into a bidirectional link
                    // widget, and copy it to "A,B" in the hash.
                    // then, we'll grab all the elements of the hash
                    // as the link widgets to draw, and append them to the
                    // widgets vector.

                    String nodeA = links[i].getSender();
                    String nodeB = links[i].getReceiver();
                    // get corresponding nodeWidgets
                    NodeWidget wA = (NodeWidget)nodeWidgets.get(nodeA);
                    NodeWidget wB = (NodeWidget)nodeWidgets.get(nodeB);
                    String xyz = ""+nodeA+","+nodeB;
                    String rev_xyz = ""+nodeB+","+nodeA;

                    LinkWidget bidiL = (LinkWidget)linkWidgets.get(rev_xyz);
                    Object lObj = links[i].getStat(property);
                    if (bidiL != null) {
                        LinkWidget newBidiL = new LinkWidget(wB,wA,
                                                             bidiL.getForwardPercent(),
                                                             links[i].getPctOfPropertyRange(property),
                                                             bidiL.getForwardQuality(),lObj);
                        linkWidgets.put(rev_xyz,newBidiL);
                        // don't do this! it's not necessary and it will cause dupe drawing!
                        //linkWidgets.put(xyz, newBidiL);
                    }
                    else {
    //                    if (nodeA.equalsIgnoreCase("mote111") && nodeB.equalsIgnoreCase("mote121")) {
    //                        LinkWidget tmpL = new LinkWidget(wA,wB,links[i].getPktPercent());
    //                        System.out.println("tmpL = "+tmpL);
    //                    }

                        // just do the forward dir for now...
                        LinkWidget newL = new LinkWidget(wA,wB,links[i].getPctOfPropertyRange(property),lObj);
                        linkWidgets.put(xyz,newL);
                    }

                }

                // now go through elements of the linkWidget hash and put them in widgets:
                int count = 0;
                for (Enumeration e1 = linkWidgets.elements(); e1.hasMoreElements(); ) {
                    widgets.add(e1.nextElement());
                    ++count;
                }
                //System.out.println("added "+count+" links!");
            }

    //        widgets.add(new LinkWidget((NodeWidget)nodeWidgets.get("mote111"),
    //                                   (NodeWidget)nodeWidgets.get("mote121"),
    //                                   .65f, .2f));
        }
        
        // grab the background image:
        this.bgImage = this.model.getBackgroundImage();
        int width = -1;
        int height = -1;
        if (this.bgImage != null) {
            width = this.bgImage.getWidth(this);
            height = this.bgImage.getHeight(this);
            
            System.out.println("NMP: stateChanged: w=" + width + ",h=" + height);
        }
        else {
            System.out.println("IMAGE WAS NULL in NMP!");
            return;
        }
        
//        int retries = 10;
//        while ((this.bgImage == null || (width <= 1 || height <= 1)) && retries > 0) {
//            
//            if (this.bgImage != null) {
//                width = bgImage.getWidth(null);
//                height = bgImage.getHeight(null);
//            }
//            else {
//                this.bgImage = this.model.getBackgroundImage();
//                width = -1;
//                height = -1;
//            }
//            
//            System.err.println("WARNING: non-null image, but no width/height info");
//            this.bgImage = null;
//            // XXX: set pref size to max posits in all directions
//
//            try {
//                Thread.currentThread().sleep(100);
//            }
//            catch (Exception ex) {
//                ;
//            }
//
//            --retries;
//        }
        
        // now, actually render the new state into the bufImage for all subsequent
        // draws:
        if (width > 1 && height > 1) {
            setPreferredSize(new Dimension(width, height));
            setMinimumSize(new Dimension(width, height));
            revalidate();
        }
        
        bufImage = (BufferedImage)createImage(width,height);
        
        if (bufImage == null) {
            System.out.println("WARNING: bufImage null!!");
            return;
        }
        Graphics2D g2 = bufImage.createGraphics();
        
        if (isOpaque()) { //paint background
            g2.setColor(getBackground());
            if (bgImage != null) {
                g2.setColor(java.awt.Color.WHITE);
                g2.fillRect(0, 0, getPreferredSize().width, getPreferredSize().height);
                //System.out.println("tried to draw bg!");
                g2.drawImage(this.bgImage,0,0,java.awt.Color.WHITE,null);
                
                
                // now draw a filled rectangle with some alpha transparency
                // to hide any hard-to-draw-over features in the map.
                // for the mote stuff, we could draw the obstacles and labels
                // ourselves, but that wouldn't help for the wireless stuff...
                
                Composite old = g2.getComposite();
                AlphaComposite ac = AlphaComposite.getInstance(AlphaComposite.SRC_OVER,0.5f);
                g2.setComposite(ac);
                
                g2.setColor(java.awt.Color.WHITE);
                g2.fillRect(0,0,getPreferredSize().width,getPreferredSize().height);
                
                g2.setComposite(old);
                
            }
            else {
                System.out.println("NMP: stateChanged drawing blank!");
                g2.fillRect(0, 0, getWidth(), getHeight());
            }
            
        }
        g2.addRenderingHints(new RenderingHints(RenderingHints.KEY_ANTIALIASING,
                                                RenderingHints.VALUE_ANTIALIAS_ON));
        
        // bold fonts
        
        for (Enumeration e1 = widgets.elements(); e1.hasMoreElements(); ) {
            Widget w = (Widget)(e1.nextElement());
            if (w instanceof NodeWidget) {
                NodeWidget nw = (NodeWidget)w;
                if (selectedNodes.contains(nw.getTitle()) && this.selectEnabled) {
                    nw.drawWidget(g2,true);
                }
                else {
                    nw.drawWidget(g2,false);
                }
            }
            else {
                w.drawWidget(g2,false);
            }
        }
        
        g2.dispose(); //clean up
        
        System.out.println("HMP: stateChanged: about to repaint");
        
        repaint();
    }
    
    protected void paintComponent(Graphics g) {
        Graphics2D g2 = (Graphics2D)g;
        
        g2.setColor(getBackground());
        g2.setColor(java.awt.Color.WHITE);
        g2.fillRect(0, 0, getWidth(), getHeight());
        if (bufImage != null) {
            g.drawImage(bufImage,0,0,null);
        }
        
    }

    // <editor-fold defaultstate="collapsed" desc=" Generated Code ">//GEN-BEGIN:initComponents
    private void initComponents() {

        setLayout(new java.awt.BorderLayout());

        setBackground(new java.awt.Color(255, 255, 255));
    }// </editor-fold>//GEN-END:initComponents
    
    class EnhancedMouseListener extends MouseAdapter implements ActionListener {
        javax.swing.Timer myTimer;
        MouseEvent cev;
        
        public void EnhancedMouseListener() {
            int multiClickInterval = ((Integer)(Toolkit.getDefaultToolkit().getDesktopProperty("awt.multiClickInterval"))).intValue();
            this.myTimer = new javax.swing.Timer(multiClickInterval,this);
        }
        
        public void actionPerformed(ActionEvent evt) {
            myTimer.stop();
            click(cev);
        }
        
        public void mousePressed(MouseEvent evt) {
            press(evt);
        }
        
        public void mouseClicked(java.awt.event.MouseEvent evt) {
            if (evt.getClickCount() > 2) {
                // ignore this one:
                return;
            }
            
            cev = evt;
            
            if (myTimer != null && myTimer.isRunning()) {
                // valid double click, don't restart the timer
                myTimer.stop();
                doubleClick(evt);
            }
            else {
                if (myTimer == null) {
                    int multiClickInterval = ((Integer)(Toolkit.getDefaultToolkit().getDesktopProperty("awt.multiClickInterval"))).intValue();
                    this.myTimer = new javax.swing.Timer(multiClickInterval,this);
                }
                myTimer.restart();
            }
        }
        
        public void click(java.awt.event.MouseEvent evt) { }
        
        public void doubleClick(java.awt.event.MouseEvent evt) { }
        
        public void press(java.awt.event.MouseEvent evt) { }
    }
    
    private void handleSingleClick(MouseEvent evt) {
        if (this.selectEnabled) {
            // find the node that was clicked:
            String clickedNode = null;
            for (Enumeration e1 = nodeWidgets.keys(); e1.hasMoreElements(); ) {
                String nodeName = (String)e1.nextElement();
                NodeWidget nw = (NodeWidget)nodeWidgets.get(nodeName);
                
                if (nw.getContainingShape().contains(evt.getX(),evt.getY())) {
                    clickedNode = nodeName;
                    break;
                }
            }
            
            if (clickedNode == null) {
                // tell controlPanel:
                this.model.setSelection(new Vector());
            }
            else {
                // if it's a click with a shift mod, add or remove from the
                //   current selection set.
                if (evt.isShiftDown()) {
                    if (this.model.getSelection().contains(clickedNode)) {
                        //this.selectedNodes.remove(clickedNode);
                        this.model.removeNodeFromSelection(clickedNode);
                    }
                    else {
                        this.model.addNodeToSelection(clickedNode);
                        //this.selectedNodes.add(clickedNode);
                    }
                    
                }
                // if it's a single click, clear the selection list
                //   adding hte node that was clicked, if any, of course
                else {
                    Vector v = new Vector();
                    v.add(clickedNode);
                    this.model.setSelection(v);
//                    if (this.selectedNodes.contains(clickedNode)) {
//                        this.selectedNodes.remove(clickedNode);
//                        this.
//                    }
//                    else {
//                        this.selectedNodes.add(clickedNode);
//                    }
                }
                
                //this.model.setSelection(this.selectedNodes);
                //this.repaint();
            }
            
        }
    }
    
    private void handleDoubleClick(MouseEvent evt) {
        // we always want to get the click at the current scale factor, then 
        // transform it into scale 1.0 coords... thus we can center any image.
        this.coordToCenter = new Point((int)(evt.getX()/this.scaleFactor),(int)(evt.getY()/scaleFactor));
        
        int scale = this.model.getScale();
            
        if (evt.isControlDown()) {
            // zoom out if possible:
            // the model knows if it's possible or not...
            this.model.setScale(scale - 1);
            //System.err.println("double click");
        }
        else {
            // zoom in if possible:
            this.model.setScale(scale + 1);
        }
        
        // reset our viewport:
        // reset the viewpoint in the containing scrollpane:
        if (this.coordToCenter != null) {
            Point nvp = new Point(0,0);
            Dimension es = this.containingSP.getViewport().getExtentSize();
            Point cvp = this.containingSP.getViewport().getViewPosition();
            
            //if (getPreferredSize().width > es.width) {
            if (this.getWidth() > es.width) {
                // scaleFactor*coordToCenter.x - es.width/2
                nvp.x = (int)(this.coordToCenter.x*this.scaleFactor - es.width/2);
            }
            if (this.getHeight() > es.height) {
                nvp.y = (int)(this.coordToCenter.y*this.scaleFactor - es.height/2);
            }
            
            // sanity check ourselves...
            if (nvp.x < 0) {
                nvp.x = 0;
            }
            else if ((nvp.x + es.getWidth()) > this.getWidth()) {
                nvp.x = (int)(this.getWidth() - es.getWidth());
            }
            if (nvp.y < 0) {
                nvp.y = 0;
            }
            else if ((nvp.y + es.getHeight()) > this.getHeight()) {
                nvp.y = (int)(this.getHeight() - es.getHeight());
            }
            
            
            this.containingSP.getViewport().setViewPosition(nvp);
            //this.containingSP.getViewport().invalidate();
        }
        
    }

    // Variables declaration - do not modify//GEN-BEGIN:variables
    // End of variables declaration//GEN-END:variables

}
