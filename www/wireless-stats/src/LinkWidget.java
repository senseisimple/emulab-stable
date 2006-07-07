/*
 * LinkWidget.java
 *
 * Created on July 11, 2005, 8:24 AM
 *
 * To change this template, choose Tools | Options and locate the template under
 * the Source Creation and Management node. Right-click the template and choose
 * Open. You can then make changes to the template in the Source Editor.
 */

import java.awt.*;
import java.awt.geom.*;

/**
 *
 * @author david
 */
public class LinkWidget extends Widget {
    
    private int x1,y1;
    private int x2,y2;
    private java.awt.geom.Line2D linkLine1To2;
    private java.awt.geom.Line2D linkLine2To1;
    private java.awt.Shape head1;
    private java.awt.Shape head2;
    private int buffer;
    private java.awt.geom.Point2D.Float p1;
    private java.awt.geom.Point2D.Float p2;
    private java.awt.geom.Point2D.Float pHalf;
    private float theta;
    private Float percent1To2;
    private Float percent2To1;
    private Object quality1To2;
    private Object quality2To1;
    private boolean bidi;
    
    private static int DEFAULT_BUFFER = 10;
    
    public String toString() {
        return "LinkWidget["+theta+"]";
    }
    
    public LinkWidget(NodeWidget nw1,NodeWidget nw2,Float percent1To2,Float percent2To1,Object linkQuality1To2,Object linkQuality2To1) {
        this(null,nw1.getX(),nw1.getY(),nw2.getX(),nw2.getY(),DEFAULT_BUFFER,percent1To2,percent2To1,linkQuality1To2,linkQuality2To1);
    }
    
    public LinkWidget(NodeWidget nw1,NodeWidget nw2,Float percent,Object linkQuality) {
        this(null,nw1.getX(),nw1.getY(),nw2.getX(),nw2.getY(),DEFAULT_BUFFER,percent,null,linkQuality,null);
    }
    
    /** Creates a new instance of LinkWidget */
    public LinkWidget(String title,int x1,int y1,int x2,int y2,int buffer,Float percent1To2,Float percent2To1,Object quality1To2,Object quality2To1) {
        super(title,0,0);
        
        
        this.x1 = x1;
        this.x2 = x2;
        this.y1 = y1;
        this.y2 = y2;
        this.buffer = buffer;
        this.percent1To2 = percent1To2;
        this.percent2To1 = percent2To1;
        this.quality1To2 = quality1To2;
        //this.quality2To1 = -1.0f;
        this.quality2To1 = quality2To1;
        this.bidi = (this.quality2To1 != null && this.quality1To2 != null)?true:false;
        
        // calculate insets based on buffer size; buffer is actually more like swing's `Insets'
//        if (x2 != x1) {
            theta = (float)Math.atan(((float)(y2-y1))/(x2-x1));
//        }
//        else {
//            System.out.println("used theta zero!");
//            theta = 0.0f;
//        }
        
        int sx = ((x2 - x1) < 0)?-1:1;
        int sy = ((y2 - y1) < 0)?-1:1;
        double dist = Math.sqrt(Math.pow((x2-x1),2) + Math.pow((y2-y1),2));
        
        p1 = new java.awt.geom.Point2D.Float((float)(x1+sx*Math.abs(buffer*Math.cos(theta))),
                                             (float)(y1+sy*Math.abs(buffer*Math.sin(theta))));
        p2 = new java.awt.geom.Point2D.Float((float)(x1+sx*Math.abs((dist-buffer)*Math.cos(theta))),
                                             (float)(y1+sy*Math.abs((dist-buffer)*Math.sin(theta))));
        pHalf = new java.awt.geom.Point2D.Float((float)(x1+sx*Math.abs((dist-buffer)*.5*Math.cos(theta))),
                                                (float)(y1+sy*Math.abs((dist-buffer)*.5*Math.sin(theta))));
        
//            lineVector = toPoint - fromPoint
//            lineLength = length of lineVector
//
//            // calculate point at base of arrowhead
//            tPointOnLine = nWidth / (2 * (tanf(fTheta) / 2) * lineLength);
//            pointOnLine = toPoint + -tPointOnLine * lineVector
//
//            // calculate left and right points of arrowhead
//            normalVector = (-lineVector.y, lineVector.x)
//            tNormal = nWidth / (2 * lineLength)
//            leftPoint = pointOnLine + tNormal * normalVector
//            rightPoint = pointOnLine + -tNormal * normalVector
        
        java.awt.geom.Point2D.Double lineVector = new java.awt.geom.Point2D.Double((p1.x - p2.x),(p1.y - p2.y));
        double lineLength = dist - buffer;
        
        double tPointOnLine = 10 / (2*(Math.tan(.785)/2)*lineLength);
        java.awt.geom.Point2D.Double pointOnLine = new java.awt.geom.Point2D.Double(
                p1.x+(-tPointOnLine*lineVector.x),
                p1.y+(-tPointOnLine*lineVector.y));
        
        java.awt.geom.Point2D.Double normalVector = new java.awt.geom.Point2D.Double(-lineVector.y,lineVector.x);
        double tNormal = 10 / (2*lineLength);
        Point lP = new Point((int)(pointOnLine.x + tNormal*normalVector.x),
                             (int)(pointOnLine.y + tNormal*normalVector.y));
        Point rP = new Point((int)(pointOnLine.x + -tNormal*normalVector.x),
                             (int)(pointOnLine.y + -tNormal*normalVector.y));
        
        
        // figure out the arrows:
        int l1x = (int)(x1+sx*Math.abs(buffer*Math.cos(theta)));
        int l1y = (int)(y1+sy*Math.abs(buffer*Math.sin(theta)));
        int x[] = new int[] { l1x,
                              lP.x,
                              rP.x
                            };
        int y[] = new int[] { l1y,
                              lP.y,
                              rP.y
                            };
        head1 = new java.awt.Polygon(x,y,3);
        
        
        lineVector = new java.awt.geom.Point2D.Double((p2.x - p1.x),(p2.y - p1.y));
        lineLength = dist - buffer;
        
        tPointOnLine = 10 / (2*(Math.tan(.785)/2)*lineLength);
        pointOnLine = new java.awt.geom.Point2D.Double(
                p2.x+(-tPointOnLine*lineVector.x),
                p2.y+(-tPointOnLine*lineVector.y));
        
        normalVector = new java.awt.geom.Point2D.Double(-lineVector.y,lineVector.x);
        tNormal = 10 / (2*lineLength);
        lP = new Point((int)(pointOnLine.x + tNormal*normalVector.x),
                             (int)(pointOnLine.y + tNormal*normalVector.y));
        rP = new Point((int)(pointOnLine.x + -tNormal*normalVector.x),
                             (int)(pointOnLine.y + -tNormal*normalVector.y));
        
        
        int l2x = (int)(x1+sx*Math.abs((dist-buffer)*Math.cos(theta)));
        int l2y = (int)(y1+sy*Math.abs((dist-buffer)*Math.sin(theta)));
        x = new int[] { l2x,
                        lP.x,
                        rP.x
                      };
        y = new int[] { l2y,
                        lP.y,
                        rP.y
                      };
        head2 = new java.awt.Polygon(x,y,3);
        
        if (!bidi) {
            linkLine1To2 = new java.awt.geom.Line2D.Float(p1,p2);
            linkLine2To1 = null;
        }
        else {
            linkLine1To2 = new java.awt.geom.Line2D.Float(pHalf,p2);
            linkLine2To1 = new java.awt.geom.Line2D.Float(pHalf,p1);
        }
    }
    
    public void drawWidget(java.awt.Graphics g,boolean selected) {
        if (g instanceof java.awt.Graphics2D) {
            Graphics2D g2 = (Graphics2D)g;
            
            Color oldColor = g2.getColor();
            
            Color titleColor = null;
            int[] defColor = { 0,0,0 };
            
            if (!bidi) {
                int[] color;
                if (this.percent1To2 != null) {
                    color = getLinkColor(this.percent1To2);
                }
                else {
                    color = defColor;
                }
                //g2.setColor(java.awt.Color.DARK_GRAY);
                g2.setColor(new Color(color[0],color[1],color[2]));
                // first draw the base line
                g2.draw(linkLine1To2);
                // fill in outline
                ;
                // draw filled heads
                //g2.fill(head1);
                g2.fill(head2);
                // draw head outlines
                ;
                titleColor = new Color(color[0],color[1],color[2]);
            }
            else {
                int[] color1To2;
                if (this.percent1To2 != null) {
                    color1To2 = getLinkColor(this.percent1To2);
                }
                else {
                    color1To2 = defColor;
                }
                g2.setColor(new Color(color1To2[0],color1To2[1],color1To2[2]));
                g2.draw(linkLine1To2);
                g2.fill(head2);
                
                int[] color2To1;
                if (this.percent2To1 != null) {
                    color2To1 = getLinkColor(this.percent2To1);
                }
                else {
                    color2To1 = defColor;
                }
                g2.setColor(new Color(color2To1[0],color2To1[1],color2To1[2]));
                g2.draw(linkLine2To1);
                g2.fill(head1);
                
                titleColor = java.awt.Color.DARK_GRAY;
            }
            // draw any text on the line!
            // first, figure out what angle to draw at:
            int dx = x2 - x1;
            int dy = y2 - y1;
            
            int avgCharSize = 6;
            if (title == null) {
                if (!bidi) {
                    //title = ""+(int)(Math.round(quality1To2*100))+" %";
                    title = quality1To2.toString();
                }
                else {
                    // gotta figure out which quality goes first:
                    Object one, two;
                    if (theta > 0) {
                        if (y1 > y2) {
                            one = quality1To2;
                            two = quality2To1;
                        }
                        else {
                            one = quality2To1;
                            two = quality1To2;
                        }
                    }
                    else {
                        if (y1 < y2) {
                            one = quality1To2;
                            two = quality2To1;
                        }
                        else {
                            one = quality2To1;
                            two = quality1To2;
                        }
                    }
                    //title = ""+(int)(Math.round(quality2To1*100))+" % / "+(int)(Math.round(quality1To2*100))+" %";
                    //title = ""+(int)(Math.round(one*100))+" % / "+(int)(Math.round(two*100))+" %";
                    title = one.toString() + " / " + two.toString();
                }
            }
            int estTitleLen = title.length()*avgCharSize;
            
            // change font
            Font oldf = g2.getFont();
            Font newf = new Font(oldf.getName(),Font.BOLD,oldf.getSize());
            g2.setFont(newf);
            
            double angle = Math.atan(((float)dy)/dx);

            int px = x1 + (int)((dx/2)) - (int)((estTitleLen/2)*Math.cos(angle)) + (int)(4*Math.sin(angle));
            int py = y1 + (int)((dy/2)) - (int)((estTitleLen/2)*Math.sin(angle)) - (int)(4*Math.cos(angle));
        
            g2.translate(px,py);
            g2.rotate(angle);
            g2.setColor(titleColor);
            g2.drawString(title,0,0);
            g2.rotate(-angle);
            g2.translate(-px,-py);
            
            g2.setFont(oldf);
            g2.setColor(oldColor);
        }
    }
    
    private int[] getLinkColor(Float p) {
        if (p != null) {
            return getLinkColor(p.floatValue());
        }
        else {
            return null;
        }
    }
    
    // returns an array of [r,g,b]
    private int[] getLinkColor(float percent) {
        int ticks = (int)((percent)*(510.0/100));
        int retvals[] = new int[3];
        retvals[1] = 0;
        retvals[0] = 255;
        retvals[2] = 0;
        
        if (ticks >= 255) {
            retvals[1] = 255;
            ticks -= 255;
            retvals[0] -= ticks;
        }
        else if (ticks > 0) {
            retvals[1] += ticks;
        }
        
        System.out.println("getLinkColor returning {"+retvals[0]+","+retvals[1]+","+retvals[2]+"}; percent was "+percent);
        
        return retvals;
    }
    
    public Object getForwardQuality() {
        return quality1To2;
    }
    
    public Object getBackwardQuality() {
        return quality2To1;
    }
    
    public Float getForwardPercent() {
        return percent1To2;
    }
    
    public Float getBackwardPercent() {
        return percent2To1;
    }
}
