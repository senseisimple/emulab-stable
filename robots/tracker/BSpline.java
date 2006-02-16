import java.awt.*;
import java.awt.geom.*;
import java.util.*;


public class BSpline implements Cloneable {
    
    private static int KNOT_RECT_LENGTH = 6;
    private static int ELLIPSE_SIZE = 6;
    
    public static int CONTINUITY_C0 = 1;
    public static int CONTINUITY_C1 = 2;
    public static int CONTINUITY_C2 = 4;
    
    private int order;
    private int continuity;
    private int num_curves;
    private double rendering_interval;
    
    private Point control[][];
    // we only have to have 2 points in the second dimension
    private Ellipse2D.Double currentHandles[][];
    private Point knots[];
    private Rectangle knotRectangles[];
    
    private int selectedKnot;
    private Point selectedKnotOrigPoint;
    // easier to keep track of this here, rather than trying to 
    // derive it from the knot.
    private int selectedHandleKnot;
    private Point selectedHandleKnotOrigPoint;
    private Point selectedKnotAdjCP0;
    private Point selectedKnotAdjCP1;
    // list which control point, the left or right (for this CURVE,
    // not the current KNOT!!!)
    private int selectedHandleLR;
    private int mouseoverKnot;
    private int mouseoverHandle;
    private int mouseoverHandleLR;
    private int drag;
    private int dragHandleOffsetX;
    private int dragHandleOffsetY;
    
    public static int SELECTION_KNOT = 1;
    public static int SELECTION_HANDLE = 2;
    
    private static int DRAG_KNOT = 1;
    private static int DRAG_CONTROL = 2;
    
    /** Creates a new instance of BSpline */
    public BSpline() {
        this(5,BSpline.CONTINUITY_C2,10,0.07);
    }
    
    /** Creates an empty BSpline.
     *
     * @param order         Order of bezier segments.
     * @param continuity    Continuity you wish enforced.
     * @param size          Reserve space for this many segments.
     * @param rendering_interval    Fidelity of rendering.
     */
    public BSpline(int order, int continuity, 
                   int size, double rendering_interval) {
        this.order = order;
        this.continuity = continuity;
        this.rendering_interval = rendering_interval;
        
        control = new Point[size][order+1];
        knots = new Point[size+1];
        knotRectangles = new Rectangle[knots.length];
        currentHandles = new Ellipse2D.Double[size+1][2];
        
        for (int i = 0; i < control.length; ++i) {
            for (int j = 0; j < control[i].length; ++j) {
                control[i][j] = null;
            }
        }
        
        for (int i = 0; i < knots.length; ++i) {
            knots[i] = null;
            knotRectangles[i] = null;
        }
        
        for (int i = 0; i < currentHandles.length; ++i) {
            for (int j = 0; j < currentHandles[i].length; ++j) {
                currentHandles[i][j] = null;
            }
        }
        
        this.num_curves = 0;
        
        this.selectedKnot = -1;
        this.selectedHandleKnot = -1;
        this.selectedHandleLR = -1;
        this.mouseoverKnot = -1;
        this.mouseoverHandle = -1;
        this.mouseoverHandleLR = -1;
        this.selectedKnotOrigPoint = null;
        this.selectedHandleKnotOrigPoint = null;
    }
    
    public void clearSelection() {
        selectedKnot = -1;
        selectedKnotOrigPoint = null;
        selectedHandleKnot = -1;
        selectedHandleKnotOrigPoint = null;
        selectedHandleLR = -1;
        mouseoverKnot = -1;
        mouseoverHandle = -1;
        mouseoverHandleLR = -1;
        drag = -1;
    }
    
    // this method auto-fills the next spline for you.
    public void addSegmentByPoint(Point endpoint) {
        
    }
    
    // this method automatically clones all points you pass, to ensure
    // that you can use them again without created an undefined state.
    public void addSegment(Point points[]) throws Exception {
        if (points.length != (order + 1)) {
            throw new Exception("Bad add: order = "+this.order+
                                ", number of points ("+points.length+
                                ") is not order + 1 !");
        }
        
        if (this.num_curves > 0 && !points[0].equals(this.knots[this.num_curves])) {
            throw new Exception("Bad add: first control point not equal"+
                                "to last knot!");
        }
        
        if (this.num_curves >= control.length) {
            Point tmp_control[][] = new Point[this.control.length+10][order+1];
            
            int i;
            for (i = 0; i < control.length; ++i) {
                for (int j = 0; j < control[i].length; ++j) {
                    tmp_control[i][j] = control[i][j];
                }
            }
            
            for ( ; i < tmp_control.length; ++i) {
                for (int j = 0; j < tmp_control[i].length; ++j) {
                    tmp_control[i][j] = null;
                }
            }
            
            control = tmp_control;
            
            // also need to update the knots list:
            Point tmp_knots[] = new Point[this.knots.length+10];
            Rectangle tmp_rects[] = new Rectangle[this.knots.length+10];
            
            for (i = 0; i < knots.length; ++i) {
                tmp_knots[i] = this.knots[i];
                tmp_rects[i] = this.knotRectangles[i];
            }
            
            for ( ; i < tmp_knots.length; ++i) {
                tmp_knots[i] = null;
                tmp_rects[i] = null;
            }
            
            knots = tmp_knots;
            knotRectangles = tmp_rects;
            
            // also need to update the currentHandles list:
            Ellipse2D.Double tmp_handles[][] = new Ellipse2D.Double[this.currentHandles.length+10][2];
            for (i = 0; i < currentHandles.length; ++i) {
                tmp_handles[i][0] = currentHandles[i][0];
                tmp_handles[i][1] = currentHandles[i][1];
            }
            
            for ( ; i < tmp_handles.length; ++i) {
                tmp_handles[i][0] = null;
                tmp_handles[i][1] = null;
            }
            
            currentHandles = tmp_handles;
        }
        
        // fix this up later, MAYBE
        for (int i = 0; i < points.length; ++i) {
            control[this.num_curves][i] = (Point)points[i].clone();
        }
        //System.out.println("did this");
        
        if (this.num_curves == 0) {
            knots[0] = (Point)points[0].clone();
            knotRectangles[0] = new Rectangle(knots[0].x - KNOT_RECT_LENGTH/2,
                                              knots[0].y - KNOT_RECT_LENGTH/2,
                                              KNOT_RECT_LENGTH,
                                              KNOT_RECT_LENGTH);
        }
        
        knots[this.num_curves+1] = (Point)points[points.length-1].clone();
        knotRectangles[this.num_curves+1] = new Rectangle(knots[this.num_curves+1].x - KNOT_RECT_LENGTH/2,
                                                          knots[this.num_curves+1].y - KNOT_RECT_LENGTH/2,
                                                          KNOT_RECT_LENGTH,
                                                          KNOT_RECT_LENGTH);
        
        ++this.num_curves;
    }
    
    public Point[] getKnots() {
        return this.knots;
    }
    
    // this does give access to the actual points so an external user
    // may easily manipulate them...
    public Point[][] getControl() {
        Point retval[][] = new Point[this.num_curves][this.order+1];
        
        for (int i = 0; i < this.num_curves; ++i) {
            retval[i] = control[i];
        }
        
        return retval;
    }
    
    public void draw(java.awt.Graphics2D g,boolean drawKnots,boolean drawAdjacentControlHandles) {
        // draw each invididual bezier curve using de Casteljau's alg
        Vector pointList = new Vector();
        Color origColor = g.getColor();
        
        for (int i = 0; i < this.num_curves; ++i) {
            double x[] = new double[control[i].length];
            double y[] = new double[control[i].length];
            
            //System.out.println("q["+i+"] = "+q);
            
            for (double t = 0.0; t <= 1.0; ) {
                for (int j = 0; j < x.length; ++j) {
                    x[j] = control[i][j].x;
                    y[j] = control[i][j].y;
                    //System.out.println("q["+z+"] = "+q[z].toString());
                }
                
                //System.out.println("t = "+t);
                for (int j = 1; j < x.length; ++j) {
                    for (int k = 0; k < (x.length - j); ++k) {
                        x[k] = (int)( (1.0 - t)*x[k] + t*x[k+1] );
                        y[k] = (int)( (1.0 - t)*y[k] + t*y[k+1] );
                        //System.out.println("q["+k+"] = "+q[k].toString());
                    }
                }
                
                pointList.add(new java.awt.geom.Point2D.Double(Math.ceil(x[0]),Math.ceil(y[0])));
                
                // make sure we get the last point.
                if (t != 1.0 && (t + this.rendering_interval) >= 1.0) {
                    t = 1.0;
                }
                else {
                    t += this.rendering_interval;
                }
            }
        }
        
        //System.out.println("pointList = "+pointList.toString());
        Point2D.Double p = null;
        Point2D.Double q = null;
        Enumeration e1 = pointList.elements();
        if (e1.hasMoreElements()) {
            q = (Point2D.Double)e1.nextElement();
        }
        
        g.setColor(java.awt.Color.BLACK);
        
        for ( ; e1.hasMoreElements(); ) {
            p = q;
            q = (Point2D.Double)e1.nextElement();
            g.draw(new Line2D.Double(p,q));
            //g.drawLine((int)q.x,(int)q.y,(int)q.x,(int)q.y);
        }
        
        if (drawKnots) {
            for (int i = 0; i < (this.num_curves + 1); ++i) {
                if (i == mouseoverKnot) {
                    g.setColor(java.awt.Color.RED);
                }
                else if (i == selectedKnot) {
                    g.setColor(java.awt.Color.BLUE);
                }
                else {
                    g.setColor(java.awt.Color.LIGHT_GRAY);
                }
                g.fill(knotRectangles[i]);
                g.setColor(java.awt.Color.BLACK);
                g.draw(knotRectangles[i]);
            }
        }
        
        if (drawAdjacentControlHandles && selectedKnot != -1) {
            // only draw the control handles at the two knots adjacent to
            // the selected knot.
            //currentHandles = new Ellipse2D.Double[3][2];
            
            //System.out.println("mHandle = "+mouseoverHandle+",mouseoverHandleLR = "+mouseoverHandleLR);
            
            int count = 0; // only want to draw max of 3 sets of control points
            for (int i = selectedKnot - 1; i < (this.num_curves + 1) && count < 3; ++i) {
                if (i < 0) {
                    ;
                }
                else if (i == 0) {
                    // only draw a single control point and line.
                    // so, if we have 6 control points, we want the 3rd one
                    // (i.e., order/2)
                    currentHandles[i][0] = null;
                    currentHandles[i][1] = 
                            new Ellipse2D.Double(control[i][this.order/2].x - this.ELLIPSE_SIZE/2,
                                                 control[i][this.order/2].y - this.ELLIPSE_SIZE/2,
                                                 this.ELLIPSE_SIZE,
                                                 this.ELLIPSE_SIZE);
                    
                    g.setColor(java.awt.Color.BLUE);
                    g.draw(new Line2D.Double(knots[i], control[i][this.order/2]));
                    
                    java.awt.Color fillColor = null;
                    if (selectedHandleKnot == i && selectedHandleLR == 1) {
                        fillColor = java.awt.Color.BLUE;
                    }
                    else if (mouseoverHandle == i && mouseoverHandleLR == 1) {
                        fillColor = java.awt.Color.RED;
                    }
                    else {
                        fillColor = java.awt.Color.WHITE;
                    }
                    
                    
                    
                    g.setColor(fillColor);
                    g.fill(currentHandles[i][1]);
                    g.setColor(java.awt.Color.BLACK);
                    g.draw(currentHandles[i][1]);
                }
                else if (i == (this.num_curves)) {
                    // only draw single control point, cause this is last knot.
                    currentHandles[i][0] = 
                            new Ellipse2D.Double(control[i-1][this.order/2+1].x - this.ELLIPSE_SIZE/2,
                                                 control[i-1][this.order/2+1].y - this.ELLIPSE_SIZE/2,
                                                 this.ELLIPSE_SIZE,
                                                 this.ELLIPSE_SIZE);
                    currentHandles[i][1] = null;
                    
                    g.setColor(java.awt.Color.BLUE);
                    g.draw(new Line2D.Double(knots[i], control[i-1][this.order/2+1]));
                    
                    java.awt.Color fillColor = null;
                    if (selectedHandleKnot == i && selectedHandleLR == 0) {
                        fillColor = java.awt.Color.BLUE;
                    }
                    else if (mouseoverHandle == i && mouseoverHandleLR == 0) {
                        fillColor = java.awt.Color.RED;
                    }
                    else {
                        fillColor = java.awt.Color.WHITE;
                    }
                    
                    g.setColor(fillColor);
                    g.fill(currentHandles[i][0]);
                    g.setColor(java.awt.Color.BLACK);
                    g.draw(currentHandles[i][0]);
                }
                else {
                    currentHandles[i][0] = 
                            new Ellipse2D.Double(control[i-1][this.order/2+1].x - this.ELLIPSE_SIZE/2,
                                                 control[i-1][this.order/2+1].y - this.ELLIPSE_SIZE/2,
                                                 this.ELLIPSE_SIZE,
                                                 this.ELLIPSE_SIZE);
                    currentHandles[i][1] = 
                            new Ellipse2D.Double(control[i][this.order/2].x - this.ELLIPSE_SIZE/2,
                                                 control[i][this.order/2].y - this.ELLIPSE_SIZE/2,
                                                 this.ELLIPSE_SIZE,
                                                 this.ELLIPSE_SIZE);
                    
                    g.setColor(java.awt.Color.BLUE);
                    g.draw(new Line2D.Double(knots[i], control[i-1][this.order/2+1]));
                    g.draw(new Line2D.Double(knots[i], control[i][this.order/2]));
                    
                    java.awt.Color fillColor0 = java.awt.Color.WHITE;
                    java.awt.Color fillColor1 = java.awt.Color.WHITE;
                    
                    if (selectedHandleKnot == i) {
                        if (selectedHandleLR == 0) {
                            fillColor0 = java.awt.Color.BLUE;
                        }
                        else if (selectedHandleLR == 1) {
                            fillColor1 = java.awt.Color.BLUE;
                        }
                    }
                    if (mouseoverHandle == i) {
                        if (mouseoverHandleLR == 0 && (selectedHandleKnot != i || selectedHandleLR != 0)) {
                            fillColor0 = java.awt.Color.RED;
                        }
                        else if (mouseoverHandleLR == 1 && (selectedHandleKnot != i || selectedHandleLR != 1)) {
                            fillColor1 = java.awt.Color.RED;
                        }
                    }
                    
                    //System.out.println("fC0 = "+fillColor0);
                    
                    g.setColor(fillColor0);
                    g.fill(currentHandles[i][0]);
                    g.setColor(fillColor1);
                    g.fill(currentHandles[i][1]);
                    g.setColor(java.awt.Color.BLACK);
                    g.draw(currentHandles[i][0]);
                    g.draw(currentHandles[i][1]);
                }
                
                
                ++count;
            }
            
            if (selectedKnot == 0) {
                
            }
        }
        
        if (false) {
            // draw all control points as pink boxes
            for (int i = 0; i < this.num_curves; ++i) {
                for (int j = 0; j < control[i].length; ++j) {
                    g.draw(new Rectangle(control[i][j].x - 2,control[i][j].y - 2,4,4));
                }
            }
        }
        
        g.setColor(origColor);
    }
    
    // call whenever including canvas is clicked... to keep selections, etc.
    public boolean notifyMouseClick(int x,int y) {
        // go through knots first...
        boolean noKnot = (selectedKnot == -1)?true:false;
        int origSelectedKnot = selectedKnot;
        selectedKnot = -1;
        
        for (int i = 0; i < (this.num_curves+1); ++i) {
            if (knotRectangles[i].contains(x,y)) {
                selectedKnot = i;
                selectedKnotOrigPoint = (Point)knots[i].clone();
                drag = DRAG_KNOT;
                
                // save dist btwn this knot and its controlpoint
                if (selectedKnot == 0) {
                    // take the left control point:
                    ;
                }
                else if (selectedKnot == this.num_curves) {
                    // take the right control point:
                    ;
                }
                else {
                    // take the left control point:
                    int main_ctl_left = (this.order/2);
                    int main_ctl_right = (this.order/2+1);
                    
                    selectedKnotAdjCP0 = (Point)control[selectedKnot-1][main_ctl_right].clone();
                    selectedKnotAdjCP1 = (Point)control[selectedKnot][main_ctl_left].clone();
                }
                
                return true;
            }
        }
        
        if (!noKnot) {
            selectedKnot = origSelectedKnot;
        }
        
        // now try control handles:
        // gotta figure out which knot to start with:
        selectedHandleKnot = -1;
        if (selectedKnot >= 0) {
            int i = selectedKnot - 1;
            int count = 0; // want to scan 3 knots.
            if (i < 0) {
                ++i;
                ++count;
            }
            for ( ; i < this.currentHandles.length && count < 3; ++i) {
                for (int j = 0; j < this.currentHandles[i].length; ++j) {
                    if (currentHandles[i][j] != null && 
                        currentHandles[i][j].contains(x,y)) {
                        // we got a match on a control handle
                        drag = DRAG_CONTROL;
                        // we shortcut this rather than figure out what exactly the control matrix
                        // value is, cause it's annoying and tedious!
                        selectedHandleKnotOrigPoint = new Point((int)(currentHandles[i][j].x+currentHandles[i][j].width/2), 
                                                                (int)(currentHandles[i][j].y+currentHandles[i][j].height/2));
                        selectedHandleKnot = i;
                        // 1 if to the right of the knot, 0 if to the left.
                        selectedHandleLR = j;
                        //if (selectedHandleLR == 0) {
                        //    --selectedHandleCurve;
                        //}
                        
                        //System.out.println("selectedHandleCurve = "+selectedHandleCurve+", LR = "+selectedHandleLR);
                        //return true;
                    }
                }
                ++count;
            }
        }
        
        if (selectedHandleKnot == -1 && !noKnot) {
            selectedKnot = -1;
            return true;
        }
        
        if (!noKnot) {
            return true;
        }

        return false;
    }
    
    // call whenever mouse moves over canvas.
    public boolean notifyMouseMotion(int x,int y) {
        boolean noKnot = (mouseoverKnot == -1)?true:false;
        mouseoverKnot = -1;
        for (int i = 0; i < (this.num_curves+1); ++i) {
            if (knotRectangles[i].contains(x,y)) {
                mouseoverKnot = i;
                //System.out.println("mouseoverKnot = "+i);
                return true;
            }
        }
        boolean noHandle = (mouseoverHandle == -1)?true:false;
        if (selectedKnot != -1) {
            // go through through the set of handles
            int count = 0;
            
            this.mouseoverHandle = -1;
            this.mouseoverHandleLR = -1;
            
            for (int i = selectedKnot - 1; i < (this.num_curves+1) && count < 3; ++i) {
                if (i < 0) {
                    ;
                }
                else if (i == 0) {
                    // only one control handle
                    if (currentHandles[i][1] != null && currentHandles[i][1].contains(x,y)) {
                        this.mouseoverHandle = i;
                        this.mouseoverHandleLR = 1;
                        return true;
                    }
                }
                else if (i == (this.num_curves+1)) {
                    // only one control handle, on the end
                    if (currentHandles[i][0] != null && currentHandles[i][0].contains(x,y)) {
                        this.mouseoverHandle = i;
                        this.mouseoverHandleLR = 0;
                        return true;
                    }
                }
                else {
                    if (currentHandles[i][1] != null && currentHandles[i][1].contains(x,y)) {
                        this.mouseoverHandle = i;
                        this.mouseoverHandleLR = 1;
                        return true;
                    }
                    else if (currentHandles[i][0] != null && currentHandles[i][0].contains(x,y)) {
                        this.mouseoverHandle = i;
                        this.mouseoverHandleLR = 0;
                        return true;
                    }
                }
                
                ++count;
            }
            
            //System.out.println("mouseoverHandle = "+mouseoverHandle+",mouseoverHandleLR"+mouseoverHandleLR);
        }
        else {
            mouseoverHandle = -1;
            mouseoverHandleLR = -1;
        }
        
        // force a repaint, cause the mouse moved OUT of the selection.
        if (!noKnot || !noHandle) {
            return true;
        }
        
        return false;
    }
    
    // call when dragging on the canvas.
    public boolean notifyDrag(int x,int y) {
        if (selectedKnot == -1 && selectedHandleKnot == -1) {
            // not our drag.  teehee...
            return false;
        }
        
        if (selectedKnot != -1 && drag == DRAG_KNOT) {
            // gotta update the coords for all knot points.
//            int delta_x = x - selectedPoint.x;
//            int delta_y = y - selectedPoint.y - y;
            
            //System.out.println("selectedKnot = "+selectedKnot);
            
            knots[selectedKnot].x = selectedKnotOrigPoint.x + x;
            knots[selectedKnot].y = selectedKnotOrigPoint.y + y;
            knotRectangles[selectedKnot].setBounds(selectedKnotOrigPoint.x + x,
                                                   selectedKnotOrigPoint.y + y,
                                                   this.KNOT_RECT_LENGTH,
                                                   this.KNOT_RECT_LENGTH);
            
            // first knot
            if (selectedKnot == 0) {
                control[0][0].x = selectedKnotOrigPoint.x + x;
                control[0][0].y = selectedKnotOrigPoint.y + y;
            }
            // last knot
            else if (selectedKnot == this.num_curves) {
                control[this.num_curves-1][control[this.num_curves-1].length-1].x = 
                        selectedKnotOrigPoint.x + x;
                control[this.num_curves-1][control[this.num_curves-1].length-1].y = 
                        selectedKnotOrigPoint.y + y;
            }
            else {
                control[selectedKnot-1][control[selectedKnot-1].length-1].x = 
                        selectedKnotOrigPoint.x + x;
                control[selectedKnot-1][control[selectedKnot-1].length-1].y = 
                        selectedKnotOrigPoint.y + y;
                
                control[selectedKnot][0].x = 
                        selectedKnotOrigPoint.x + x;
                control[selectedKnot][0].y = 
                        selectedKnotOrigPoint.y + y;
                
                // shift the adjacent handle points to maintain c2 cont;
                // if we do this here, then the code below will maintain the
                // alignment of the control points between the handles and
                // knots.
                if (this.continuity == BSpline.CONTINUITY_C2) {
                    int main_ctl_left = (this.order/2);
                    int main_ctl_right = (this.order/2+1);
                    
                    control[selectedKnot-1][main_ctl_right].x = selectedKnotAdjCP0.x + x;
                    control[selectedKnot-1][main_ctl_right].y = selectedKnotAdjCP0.y + y;
                    
                    control[selectedKnot][main_ctl_left].x = selectedKnotAdjCP1.x + x;
                    control[selectedKnot][main_ctl_left].y = selectedKnotAdjCP1.y + y;
                }
                
            }
            // now gotta shift all the control points adjacent to 
            // this knot (control) point.
            // if it's a 5th order b-spline, we want to shift the 2nd and 5th
            // control points to be on lines (1st,3rd) and (4th,6th).
            // Basically, for any order spline, we gotta shift all points
            // between the knot point and the topmost control point (i.e., 
            // either order/2+1 to knot, or knot to order/2).
            
            if (selectedKnot == 0) {
                // only shift the control points between knot 0 and ...
                // this formula will ensure that we shift exactly 1 pt on the
                // left side and one on the right for a 5th order spline.
                // if you use 4th order, only one point on the left will shift;
                // nothing on the right will shift.
                // so, must be consistent.
                int main_ctl_left = (this.order/2);
                int main_ctl_right = (this.order/2+1);
                
                int num_shift = main_ctl_left - 1;
                int divide_factor = num_shift + 1;
                int delta_x = control[0][main_ctl_left].x - control[0][0].x;
                int delta_y = control[0][main_ctl_left].y - control[0][0].y;
                
                int count = 1;
                for (int i = 1; i < (1 + num_shift); ++i) {
                    control[0][i].x = control[0][0].x + count*(delta_x/divide_factor);
                    control[0][i].y = control[0][0].y + count*(delta_y/divide_factor);
                    ++count;
                }
            }
            else if (selectedKnot == this.num_curves) {
                int main_ctl_left = (this.order/2);
                int main_ctl_right = (this.order/2+1);
                
                int num_shift = (control[0].length-1) - (main_ctl_right+1);
                int divide_factor = num_shift + 1;
                int delta_x = control[this.num_curves-1][control[this.num_curves-1].length-1].x - control[this.num_curves-1][main_ctl_right].x;
                int delta_y = control[this.num_curves-1][control[this.num_curves-1].length-1].y - control[this.num_curves-1][main_ctl_right].y;
                
                int count = 1;
                for (int i = main_ctl_right + 1; i < (main_ctl_right + 1 + num_shift); ++i) {
                    // i = 4
                    //System.out.println("i = "+i);
                    control[this.num_curves-1][i].x = control[this.num_curves-1][main_ctl_right].x + count*(delta_x/divide_factor);
                    control[this.num_curves-1][i].y = control[this.num_curves-1][main_ctl_right].y + count*(delta_y/divide_factor);
                    ++count;
                }
            }
            else {
                // ok, now we have to do both sides of a middle knot.
                // first, the left side:
                int main_ctl_left = (this.order/2);
                int main_ctl_right = (this.order/2+1);
                
                // second, the right side:
                int num_shift = (control[selectedKnot-1].length-1) - (main_ctl_right+1);
                int divide_factor = num_shift + 1;
                int delta_x = control[selectedKnot-1][control[selectedKnot-1].length-1].x - control[selectedKnot-1][main_ctl_right].x;
                int delta_y = control[selectedKnot-1][control[selectedKnot-1].length-1].y - control[selectedKnot-1][main_ctl_right].y;
                
                int count = 1;
                for (int i = main_ctl_right + 1; i < (main_ctl_right + 1 + num_shift); ++i) {
                    //System.out.println("i = "+i);
                    control[selectedKnot-1][i].x = control[selectedKnot-1][main_ctl_right].x + count*(delta_x/divide_factor);
                    control[selectedKnot-1][i].y = control[selectedKnot-1][main_ctl_right].y + count*(delta_y/divide_factor);
                    ++count;
                }
                
                num_shift = main_ctl_left - 1;
                divide_factor = num_shift + 1;
                delta_x = control[selectedKnot][main_ctl_left].x - control[selectedKnot][0].x;
                delta_y = control[selectedKnot][main_ctl_left].y - control[selectedKnot][0].y;
                
                count = 1;
                for (int i = 1; i < (1 + num_shift); ++i) {
                    control[selectedKnot][i].x = control[selectedKnot][0].x + count*(delta_x/divide_factor);
                    control[selectedKnot][i].y = control[selectedKnot][0].y + count*(delta_y/divide_factor);
                    ++count;
                }
                
                
            }
            
        }
        
        // XXXXXXXXXXXXXXXXXXX
        
        // reminder: need to clear out selectedKnot... wait, put this ahead of 
        // seelcted knot? hmmm
        if (selectedHandleKnot != -1 && drag == DRAG_CONTROL) {
            // only have to make sure that we keep any other control points
            // between this one and its knot on the same line.
            int main_ctl_left = (this.order/2);
            int main_ctl_right = (this.order/2+1);
            
            
            
//            control[selectedHandleKnot].x = selectedHandleKnotOrigPoint.x + x;
//            knots[selectedHandleKnot].y = selectedHandleKnotOrigPoint.y + y;
//            knotRectangles[selectedHandleKnot].setBounds(selectedHandleKnotOrigPoint.x + x,
//                                                   selectedHandleKnotOrigPoint.y + y,
//                                                   this.KNOT_RECT_LENGTH,
//                                                   this.KNOT_RECT_LENGTH);
            
            // first knot
            if (selectedHandleKnot == 0) {
                control[0][main_ctl_left].x = selectedHandleKnotOrigPoint.x + x;
                control[0][main_ctl_left].y = selectedHandleKnotOrigPoint.y + y;
            }
            // last knot
            else if (selectedHandleKnot == this.num_curves) {
                control[this.num_curves-1][main_ctl_right].x = 
                        selectedHandleKnotOrigPoint.x + x;
                control[this.num_curves-1][main_ctl_right].y = 
                        selectedHandleKnotOrigPoint.y + y;
            }
            else {
                int control_idx = selectedHandleKnot - 1;
                
                
                if (selectedHandleLR == 1) {
                    //++control_idx;
                
                    control[control_idx+1][main_ctl_left].x = 
                            selectedHandleKnotOrigPoint.x + x;
                    control[control_idx+1][main_ctl_left].y = 
                            selectedHandleKnotOrigPoint.y + y;
                }
                else {
                    control[control_idx][main_ctl_right].x = 
                            selectedHandleKnotOrigPoint.x + x;
                    control[control_idx][main_ctl_right].y = 
                            selectedHandleKnotOrigPoint.y + y;
                }
                
                //System.out.println("selectedKnot = "+selectedKnot+", selectedHandleKnot = "+selectedHandleKnot);
                
                // enforce continuity on the other side of the knot:
                if (this.continuity == BSpline.CONTINUITY_C2) {
                    int xdiff, ydiff;
                
                    xdiff = knots[selectedHandleKnot].x - (selectedHandleKnotOrigPoint.x + x);
                    ydiff = knots[selectedHandleKnot].y - (selectedHandleKnotOrigPoint.y + y);
                
                    if (selectedHandleLR == 1) {
                        control[control_idx][main_ctl_right].x = 
                                knots[selectedHandleKnot].x + xdiff;
                        control[control_idx][main_ctl_right].y = 
                                knots[selectedHandleKnot].y + ydiff;
                    }
                    else if (selectedHandleLR == 0) {
                        control[control_idx+1][main_ctl_left].x = 
                                knots[selectedHandleKnot].x + xdiff;
                        control[control_idx+1][main_ctl_left].y = 
                                knots[selectedHandleKnot].y + ydiff;
                    }
                    
                    // now we gotta shift the control points between the 
                    // selected knot and the control point on the OTHER side
                    // of the knot:
                    
                    // OOPS, this gets done below where I shift the control points
                    // on BOTH sides of the knots.  I was thinking ahead subconciously!!!
                    
                    
                }
                
            }
            
            
            // shifting control points between the selected handle and the knot
            if (selectedHandleKnot == 0) {
                // only shift the control points between knot 0 and ...
                // this formula will ensure that we shift exactly 1 pt on the
                // left side and one on the right for a 5th order spline.
                // if you use 4th order, only one point on the left will shift;
                // nothing on the right will shift.
                // so, must be consistent.
                
                
                int num_shift = main_ctl_left - 1;
                int divide_factor = num_shift + 1;
                int delta_x = control[0][main_ctl_left].x - control[0][0].x;
                int delta_y = control[0][main_ctl_left].y - control[0][0].y;
                
                int count = 1;
                for (int i = 1; i < (1 + num_shift); ++i) {
                    control[0][i].x = control[0][0].x + count*(delta_x/divide_factor);
                    control[0][i].y = control[0][0].y + count*(delta_y/divide_factor);
                    ++count;
                }
            }
            else if (selectedHandleKnot == this.num_curves) {
                
                int num_shift = (control[0].length-1) - (main_ctl_right+1);
                int divide_factor = num_shift + 1;
                int delta_x = control[this.num_curves-1][control[this.num_curves-1].length-1].x - control[this.num_curves-1][main_ctl_right].x;
                int delta_y = control[this.num_curves-1][control[this.num_curves-1].length-1].y - control[this.num_curves-1][main_ctl_right].y;
                
                int count = 1;
                for (int i = main_ctl_right + 1; i < (main_ctl_right + 1 + num_shift); ++i) {
                    // i = 4
                    //System.out.println("i = "+i);
                    control[this.num_curves-1][i].x = control[this.num_curves-1][main_ctl_right].x + count*(delta_x/divide_factor);
                    control[this.num_curves-1][i].y = control[this.num_curves-1][main_ctl_right].y + count*(delta_y/divide_factor);
                    ++count;
                }
            }
            else {
                // ok, now we have to do both sides of a middle knot.
                // first, the left side:
                
                // second, the right side:
                int num_shift = (control[selectedHandleKnot-1].length-1) - (main_ctl_right+1);
                int divide_factor = num_shift + 1;
                int delta_x = control[selectedHandleKnot-1][control[selectedHandleKnot-1].length-1].x - control[selectedHandleKnot-1][main_ctl_right].x;
                int delta_y = control[selectedHandleKnot-1][control[selectedHandleKnot-1].length-1].y - control[selectedHandleKnot-1][main_ctl_right].y;
                
                int count = 1;
                for (int i = main_ctl_right + 1; i < (main_ctl_right + 1 + num_shift); ++i) {
                    //System.out.println("i = "+i);
                    control[selectedHandleKnot-1][i].x = control[selectedHandleKnot-1][main_ctl_right].x + count*(delta_x/divide_factor);
                    control[selectedHandleKnot-1][i].y = control[selectedHandleKnot-1][main_ctl_right].y + count*(delta_y/divide_factor);
                    ++count;
                }
                
                num_shift = main_ctl_left - 1;
                divide_factor = num_shift + 1;
                delta_x = control[selectedHandleKnot][main_ctl_left].x - control[selectedHandleKnot][0].x;
                delta_y = control[selectedHandleKnot][main_ctl_left].y - control[selectedHandleKnot][0].y;
                
                count = 1;
                for (int i = 1; i < (1 + num_shift); ++i) {
                    control[selectedHandleKnot][i].x = control[selectedHandleKnot][0].x + count*(delta_x/divide_factor);
                    control[selectedHandleKnot][i].y = control[selectedHandleKnot][0].y + count*(delta_y/divide_factor);
                    ++count;
                }
                
                
            }
            
            
        }
        
        return true;
    }
    
    protected Object clone() {
        BSpline obj = new BSpline(this.order,this.continuity,
                                  this.control.length,this.rendering_interval);
        
        for (int i = 0; i < control.length; ++i) {
            try {
                obj.addSegment(control[i]);
            }
            // theoretically, this cannot happen, since these arrays come from
            // a spline that was already created, presumably without exception.
            catch (Exception e) {
                obj = null;
                e.printStackTrace();
            }
        }
        
        return obj;
    }
    
}
