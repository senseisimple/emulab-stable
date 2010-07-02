/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004-2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

import java.awt.*;
import java.awt.geom.*;
import java.util.*;


public class DrawPanel extends javax.swing.JPanel {
    
    public static int TOOL_NONE = 0;
    public static int TOOL_DRAW_SPLINE = 2;
    public static int TOOL_EDIT_SPLINE = 4;
    public static int TOOL_DELETE_SPLINE = 8;
    
    private Vector splines;
    private BSpline currentEdit;
    private int tool;
    
    /** Creates new form DrawPanel */
    public DrawPanel() {
        initComponents();
        
        splines = new Vector();
        currentEdit = null;
        
//        BSpline bs = new BSpline(3, 2, 10, 0.07);
//        Point p[] = new Point[4];
//        p[0] = new Point(50,200);
//        p[1] = new Point(120,100);
//        p[2] = new Point(180,100);
//        p[3] = new Point(250,200);
//        try {
//            bs.addSegment(p);
//        }
//        catch (Exception e) {
//            e.printStackTrace();
//        }
        
        int xshift = 0;
        
        BSpline bs2 = new BSpline(5, BSpline.CONTINUITY_C2, 10, 0.07);
        Point p2[] = new Point[6];
        p2[0] = new Point(xshift+50,200);
         p2[1] = new Point(xshift+85,150);
        p2[2] = new Point(xshift+120,100);
        p2[3] = new Point(xshift+180,100);
         p2[4] = new Point(xshift+215,150);
        p2[5] = new Point(xshift+250,200);
        try {
            bs2.addSegment(p2);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        
        
        
        xshift = 200;
        
        //bs2 = new BSpline(5, 2, 10, 0.08);
        Point p3[] = new Point[6];
        p3[0] = new Point(xshift+50,200);
         p3[1] = new Point(xshift+85,150);
        p3[2] = new Point(xshift+120,100);
        p3[3] = new Point(xshift+180,100);
         p3[4] = new Point(xshift+215,150);
        p3[5] = new Point(xshift+250,200);
        try {
            bs2.addSegment(p3);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        
        //this.addSpline(bs);
        this.addSpline(bs2);
    }
    
    /** This method is called from within the constructor to
     * initialize the form.
     * WARNING: Do NOT modify this code. The content of this method is
     * always regenerated by the Form Editor.
     */
    // <editor-fold defaultstate="collapsed" desc=" Generated Code ">//GEN-BEGIN:initComponents
    private void initComponents() {

        setLayout(new java.awt.BorderLayout());

        setBackground(new java.awt.Color(255, 255, 255));
        setPreferredSize(new java.awt.Dimension(250, 200));
        addMouseMotionListener(new java.awt.event.MouseMotionAdapter() {
            public void mouseDragged(java.awt.event.MouseEvent evt) {
                panelMouseDragged(evt);
            }
            public void mouseMoved(java.awt.event.MouseEvent evt) {
                panelMouseMoved(evt);
            }
        });
        addMouseListener(new java.awt.event.MouseAdapter() {
            public void mouseClicked(java.awt.event.MouseEvent evt) {
                panelMouseClicked(evt);
            }
            public void mousePressed(java.awt.event.MouseEvent evt) {
                panelMousePressed(evt);
            }
            public void mouseReleased(java.awt.event.MouseEvent evt) {
                panelMouseReleased(evt);
            }
        });

    }
    // </editor-fold>//GEN-END:initComponents

    private int drag_start_x;
    private int drag_start_y;
    private BSpline drag_spline;
    
    private void panelMouseReleased(java.awt.event.MouseEvent evt) {//GEN-FIRST:event_panelMouseReleased
        // TODO add your handling code here:
    }//GEN-LAST:event_panelMouseReleased

    private void panelMousePressed(java.awt.event.MouseEvent evt) {//GEN-FIRST:event_panelMousePressed
        if (tool == TOOL_EDIT_SPLINE) {
            drag_start_x = evt.getX();
            drag_start_y = evt.getY();
            
            for (Enumeration e1 = splines.elements(); e1.hasMoreElements(); ) {
                BSpline bs = (BSpline)e1.nextElement();
                if (bs.notifyMouseClick(evt.getX(),evt.getY())) {
                    drag_spline = bs;
                    this.repaint();
                    break;
                }
            }
        }
        
    }//GEN-LAST:event_panelMousePressed

    private void panelMouseDragged(java.awt.event.MouseEvent evt) {//GEN-FIRST:event_panelMouseDragged
        if (tool == TOOL_EDIT_SPLINE) {
            if ( drag_spline != null && 
                 drag_spline.notifyDrag(evt.getX()-drag_start_x,evt.getY()-drag_start_y) ) {
                this.repaint();
            }
        }
    }//GEN-LAST:event_panelMouseDragged

    private void panelMouseClicked(java.awt.event.MouseEvent evt) {//GEN-FIRST:event_panelMouseClicked
        if (tool == TOOL_EDIT_SPLINE) {
            for (Enumeration e1 = splines.elements(); e1.hasMoreElements(); ) {
                BSpline bs = (BSpline)e1.nextElement();
                if (bs.notifyMouseClick(evt.getX(),evt.getY())) {
                    this.repaint();
                }
            }
        }
        else if (tool == TOOL_DRAW_SPLINE) {
            ;
        }
        else if (tool == TOOL_DELETE_SPLINE) {
            for (Enumeration e1 = splines.elements(); e1.hasMoreElements(); ) {
                BSpline bs = (BSpline)e1.nextElement();
                if (bs.notifyMouseClick(evt.getX(),evt.getY())) {
                    splines.remove(bs);
                    this.repaint();
                }
            }
        }
    }//GEN-LAST:event_panelMouseClicked

    private void panelMouseMoved(java.awt.event.MouseEvent evt) {//GEN-FIRST:event_panelMouseMoved
        if (tool == TOOL_EDIT_SPLINE) {
            for (Enumeration e1 = splines.elements(); e1.hasMoreElements(); ) {
                BSpline bs = (BSpline)e1.nextElement();
                if (bs.notifyMouseMotion(evt.getX(),evt.getY())) {
                    this.repaint();
                }
            }
        }
    }//GEN-LAST:event_panelMouseMoved
    
    public void setTool(int tool) {
        for (Enumeration e1 = splines.elements(); e1.hasMoreElements(); ) {
            BSpline bs = (BSpline)e1.nextElement();
            bs.clearSelection();
            this.repaint();
        }
        
        this.repaint();
        
        if (this.tool == TOOL_DRAW_SPLINE) {
            // finalize the draw:
            
        }
        
        this.tool = tool;
    }
    
    public void addSpline(BSpline bs) {
        splines.add(bs);
    }
    
    protected void paintComponent(Graphics g) {
        Graphics2D g2 = (Graphics2D)g;
        
        if (isOpaque()) { //paint background
            g2.setColor(getBackground());
            g2.fillRect(0, 0, getWidth(), getHeight());
        }
        g2.addRenderingHints(new RenderingHints(RenderingHints.KEY_ANTIALIASING,
                                                RenderingHints.VALUE_ANTIALIAS_ON));

        
        for (Enumeration e1 = splines.elements(); e1.hasMoreElements(); ) {
            BSpline bs = (BSpline)e1.nextElement();
            bs.draw(g2,true,true);
        }
        
        // now, if necessary, draw the spline being modified!
        // maybe only draw the segments that can move...
        if (currentEdit != null) {
            currentEdit.draw(g2,false,false);
        }
        
        g2.dispose();
    }
    
    public Dimension getPreferredSize() {
        return new Dimension(1000,500);
    }
    
    // Variables declaration - do not modify//GEN-BEGIN:variables
    // End of variables declaration//GEN-END:variables
    
}
