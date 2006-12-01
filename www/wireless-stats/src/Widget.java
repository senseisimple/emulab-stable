/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005-2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

import java.util.*;
import java.awt.*;

/*
 * Widget.java
 *
 * Created on July 11, 2005, 8:26 AM
 *
 * To change this template, choose Tools | Options and locate the template under
 * the Source Creation and Management node. Right-click the template and choose
 * Open. You can then make changes to the template in the Source Editor.
 */

/**
 *
 * @author david
 */
public class Widget {
    
    protected String title;
    private boolean titleVisible;
    private Hashtable properties;
    private boolean visible;
    private int x;
    private int y;
    
    public static Color selectedColor = new Color(37,160,231);
    
    /** Creates a new instance of Widget */
    public Widget() {
        this.properties = new Hashtable();
        this.title = null;
        this.x = 100;
        this.y = 100;
    }
    
    public Widget(String title,int x,int y) {
        this.properties = new Hashtable();
        this.title = title;
        this.x = x;
        this.y = y;
    }
    
    public int getX() {
        return x;
    }
    
    public int getY() {
        return y;
    }
    
    public String getTitle() {
        return title;
    }
    
    public void setX(int x) {
        this.x = x;
    }
    
    public void setY(int y) {
        this.y = y;
    }
    
    public void setLocation(java.awt.Point p) {
        this.x = p.x;
        this.y = p.y;
    }
    
    public void setTitle(String title) {
        this.title = title;
    }
    
    public void setVisible(boolean vis) {
        this.visible = vis;
    }
    
    public void setTitleVisible(boolean vis) {
        this.titleVisible = vis;
    }
    
    public Shape getContainingShape() {
        return new java.awt.geom.Ellipse2D.Float(x-4,y-4,8,8);
    }
    
    public void drawWidget(java.awt.Graphics g,boolean selected) {
        if (g instanceof java.awt.Graphics2D) {
            Graphics2D g2 = (Graphics2D)g;
            java.awt.geom.Ellipse2D e = new java.awt.geom.Ellipse2D.Float(x-4,y-4,8,8);
            Color oldColor = g2.getColor();
            if (selected) {
                g2.setColor(selectedColor);
            }
            else {
                g2.setColor(java.awt.Color.DARK_GRAY);
            }
            g2.fill(e);
            g2.setColor(java.awt.Color.BLACK);
            g2.draw(e);
            g2.setColor(java.awt.Color.DARK_GRAY);
            //g2.rotate(0.52360);
            Font oldf = g2.getFont();
            Font newf = new Font(oldf.getName(),Font.BOLD,oldf.getSize());
            g2.setFont(newf);
            int avgCharSize = 6;
            int estTitleLen = title.length()*avgCharSize;
            //g2.drawString(this.title,this.x - estTitleLen/2,this.y - 16);
            // fix from Russ to ensure draw on canvas...
            int x = this.x - estTitleLen/2;
     	    if ( x < 0 ) x = 0;	// Left edge.
            int y = this.y - 16;
            if ( y < 16 ) y = this.y + 24; // Top edge.
            g2.drawString(this.title,x,y);
            //g2.rotate(-0.52360);
            g2.setFont(oldf);
            g2.setColor(oldColor);
        }
    }
    
}
