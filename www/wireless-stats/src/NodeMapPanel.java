/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

import javax.swing.*;
import javax.swing.event.*;
import java.awt.*;
import java.awt.event.*;
import java.util.*;

public class NodeMapPanel extends javax.swing.JPanel implements ChangeListener {
    
    private Image bgImage;
    // the widgets vector is what actually gets drawn, but the elements
    // in the following hashtables are provided for ease of ref
    private Hashtable nodeWidgets;
    private Vector selectedNodes;
    private boolean selectEnabled;
    private Hashtable linkWidgets;
    private Vector widgets;
    private MapDataModel model;
    private ControlPanel controlPanel;
    private float scaleFactor;
    
    /**
     * Creates new form NodeMapPanel 
     */
    public NodeMapPanel() {
        super();
        
        this.widgets = new Vector();
        this.bgImage = null;
        this.nodeWidgets = new Hashtable();
        this.linkWidgets = new Hashtable();
        this.model = null;
        this.selectedNodes = new Vector();
        this.selectEnabled = false;
        
        setPreferredSize(new Dimension(800,600));
        
        initComponents();
        
        //setOpaque(true);
    }
    
    public void setModel(MapDataModel model) {
        if (this.model != null) {
            // remove ourselves as a listener, just in case.
            this.model.removeChangeListener(this);
        }
        
        this.model = model;
        
        this.model.addChangeListener(this);
    }
    
    public void setControlPanel(ControlPanel controlPanel) {
        this.controlPanel = controlPanel;
    }
    
    public void controlPanelSetSelectedNodes(Vector nodes) {
        this.selectedNodes = nodes;
        System.out.println("CP sent sel nodes to map: "+nodes.toString());
        this.repaint();
    }
    
    public void controlPanelSetSelectEnabled(boolean val) {
        this.selectEnabled = val;
    }
    
    public void stateChanged(ChangeEvent e) {
        // here, we'll have to read the whole state of the data model, 
        // and figure out what to draw.  Probably best if the model
        // has some functions to tell us which nodes and links to draw.
        
//        if (true) {
//            return;
//        }
        
        this.scaleFactor = this.model.getScaleFactor();
        
        this.widgets.clear();
        this.nodeWidgets.clear();
        this.linkWidgets.clear();
        
        String[] nodes = model.getNodes();
        if (nodes != null) {
            for (int i = 0; i < nodes.length; ++i) {
                System.out.println("nodes["+i+"] = '"+nodes[i]+"'");
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
        
        this.repaint();
    }

    // <editor-fold defaultstate="collapsed" desc=" Generated Code ">//GEN-BEGIN:initComponents
    private void initComponents() {

        setLayout(new java.awt.BorderLayout());

        setBackground(new java.awt.Color(255, 255, 255));
        addMouseListener(new java.awt.event.MouseAdapter() {
            public void mouseClicked(java.awt.event.MouseEvent evt) {
                formMouseClicked(evt);
            }
        });

    }
    // </editor-fold>//GEN-END:initComponents

    private void formMouseClicked(java.awt.event.MouseEvent evt) {//GEN-FIRST:event_formMouseClicked
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
            
            if (clickedNode == null && this.selectedNodes.size() > 0) {
                this.selectedNodes.clear();
                // tell controlPanel:
                this.controlPanel.mapSetSelectedNodes(this.selectedNodes);
                this.repaint();
            }
            else if (clickedNode == null) {
                ;
            }
            else {
                // if it's a click with a shift mod, add or remove from the
                //   current selection set.
                if (evt.isShiftDown()) {
                    if (this.selectedNodes.contains(clickedNode)) {
                        this.selectedNodes.remove(clickedNode);
                    }
                    else {
                        this.selectedNodes.add(clickedNode);
                    }
                }
                // if it's a single click, clear the selection list
                //   adding hte node that was clicked, if any, of course
                else {
                    this.selectedNodes.clear();
                    this.selectedNodes.add(clickedNode);
//                    if (this.selectedNodes.contains(clickedNode)) {
//                        this.selectedNodes.remove(clickedNode);
//                        this.
//                    }
//                    else {
//                        this.selectedNodes.add(clickedNode);
//                    }
                }
                
                this.controlPanel.mapSetSelectedNodes(this.selectedNodes);
                this.repaint();
            }
            
        }
    }//GEN-LAST:event_formMouseClicked
    
    
    // Variables declaration - do not modify//GEN-BEGIN:variables
    // End of variables declaration//GEN-END:variables
    protected void paintComponent(Graphics g) {
        Graphics2D g2 = (Graphics2D)g;
        
        if (isOpaque()) { //paint background
            g2.setColor(getBackground());
            if (bgImage != null) {
                g2.setColor(java.awt.Color.WHITE);
                g2.fillRect(0, 0, getWidth(), getHeight());
                //System.out.println("tried to draw bg!");
                g2.drawImage(bgImage,0,0,java.awt.Color.WHITE,null);
                
                
                // now draw a filled rectangle with some alpha transparency
                // to hide any hard-to-draw-over features in the map.
                // for the mote stuff, we could draw the obstacles and labels
                // ourselves, but that wouldn't help for the wireless stuff...
                
                Composite old = g2.getComposite();
                AlphaComposite ac = AlphaComposite.getInstance(AlphaComposite.SRC_OVER,0.5f);
                g2.setComposite(ac);
                
                g2.setColor(java.awt.Color.WHITE);
                g2.fillRect(0,0,getWidth(),getHeight());
                
                g2.setComposite(old);
                
            }
            else {
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
    }
    
    public void setScaleFactor(float f) {
        this.scaleFactor = f;
    }
    
    public void setBackgroundImage(final Image bgImage) {
        this.bgImage = bgImage;
        
        java.awt.image.ImageObserver io = new java.awt.Component() {
            public boolean updateImage(Image img, int infoflags,
                                       int x, int y, 
                                       int width, int height) {
                System.out.println("ImageObserver w = "+width+",h = "+height);
                
                return true;
            }
        };
        
        int width = bgImage.getWidth(null);
        int height = bgImage.getHeight(null);
        System.out.println("sbi: width = "+width+", height = "+height);
        setPreferredSize(new Dimension(width, height));
        setMinimumSize(new Dimension(width, height));
        revalidate();
        repaint();
    }
    
}
