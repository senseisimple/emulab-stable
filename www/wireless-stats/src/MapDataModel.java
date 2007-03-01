/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006-2007 University of Utah and the Flux Group.
 * All rights reserved.
 */

import java.util.*;
import javax.swing.event.*;
import java.awt.Point;


public class MapDataModel {
    
    // these are the different display modes:
    public static int MODE_ALL = 1;
    public static int MODE_SELECT_SRC = 2;
    public static int MODE_SELECT_DST = 4;
    public static int MODE_MST = 8;
    
    public static int LIMIT_NONE = 64;
    public static int LIMIT_N_BEST_NEIGHBOR = 128;
    public static int LIMIT_THRESHOLD = 256;
    
    public static Integer OPTION_NO_ZERO_LINKS = new Integer(1);
    
    public static Boolean OPTION_SET = new Boolean(true);
    public static Boolean OPTION_UNSET = new Boolean(false);
    
    private GenericWirelessData data;
    private Vector selectionList;
    private Object threshold;
    private int neighborCount;
    private Vector changeListeners;
    private int mode;
    private int limit;
    private NodePosition positions;
    
    // new for generic...
    private String[] currentIndexValues;
    private String currentProperty;
    
    private Hashtable options;
    
    private int currentFloor;
    private int currentScale;
    private int minScale;
    private int maxScale;
    
    private Dataset dataset;

    
    private MapDataModel() {
        this(new Dataset());
    }
    
    public MapDataModel(Dataset ds) {
        this.dataset = ds;
        this.data = ds.data;
        this.positions = ds.positions;
        selectionList = new Vector();
        threshold = new Float(0);
        neighborCount = 3;
        changeListeners = new Vector();
        
        // set the currentIndexValues to the first value in each list of
        // possible index values:
        String[] ia = data.getIndices();
        currentIndexValues = new String[ia.length];
        for (int i = 0; i < ia.length; ++i) {
            Vector tv = data.getIndexValues(ia[i]);
            currentIndexValues[i] = (String)tv.firstElement();
        }
        
        // set the currentProperty to the first value:
        Vector tv = data.getProperties();
        currentProperty = (String)tv.firstElement();
        
        mode = MODE_ALL;
        limit = LIMIT_NONE;
        this.options = new Hashtable();
        options.put(OPTION_NO_ZERO_LINKS,OPTION_SET);
        
        // find min/max scale items:
        int min = 65535;
        this.minScale = -1;
        int max = -65535;
        this.maxScale = -1;
        for (int i = 0; i < ds.scale.length; ++i) {
            if (ds.scale[i] < min) {
                min = ds.scale[i];
                this.minScale = min;
            }
            if (ds.scale[i] > max) {
                max = ds.scale[i];
                this.maxScale = max;
            }
        }
        this.currentScale = this.minScale;
        //System.err.println("model set minScale="+minScale+",maxScale"+maxScale+",currentScale="+currentScale);
        //System.err.println("model set min="+min+",max"+max);
        
        // now set floor:
        // just take the first one :-)
        this.currentFloor = ds.floor[0];
        //System.err.println("model set currentFloor="+currentFloor);
        
    }
    
    public java.awt.Image getBackgroundImage() {
        java.awt.Image i = (java.awt.Image)this.dataset.getImage(this.currentFloor,
                                                                 this.currentScale);
        while (i.getWidth(null) <= 1 || i.getHeight(null) <= 1) {
            try {
                Thread.currentThread().sleep(10);
                System.out.println("DEBUG: waiting for image to have valid w/h!");
            }
            catch (Exception ex) {
                ;
            }
        }
        
        return i;
    }
    
    public Float getCurrentPropertyDelta() {
        Object min = this.data.getMinPropertyValue(currentProperty);
        Object max = this.data.getMaxPropertyValue(currentProperty);
        if ((min != null && max != null) 
            && (min instanceof Float && max instanceof Float)) {
            return new Float(((Float)max).floatValue() - ((Float)min).floatValue());
        }
        
        return null;
    }
    
    // this is a nice helpful method for the nodemappanel... so it doesn't have
    // to be aware of what kind of functionality this model exposes.
    protected String[] getNodes() {
        return data.getNodes();
    }
    
    private GenericLinkStats[] filterAccordingToLimit(GenericLinkStats links[]) {
        GenericLinkStats retval[] = null;
        Vector tmp = new Vector();
        
        if (limit == LIMIT_NONE) {
            // they all pass...
            retval = links;
        }
        else if (limit == LIMIT_THRESHOLD) {
            // process out the chaff according to the limit
            for (int i = 0; i < links.length; ++i) {
                Object linkVal = links[i].getStat(this.currentProperty);
                if (this.threshold.getClass() == linkVal.getClass()
                    && linkVal instanceof Comparable
                    && ((Comparable)linkVal).compareTo(this.threshold) >= 0) {
                    tmp.add(links[i]);
                }
            }
            // dump back to array...
            retval = new GenericLinkStats[tmp.size()];
            int i = 0;
            for (Enumeration e1 = tmp.elements(); e1.hasMoreElements(); ) {
                retval[i] = (GenericLinkStats)e1.nextElement();
                ++i;
            }
        }
        else if (limit == LIMIT_N_BEST_NEIGHBOR) {
            retval = null;

            //System.out.println("neighborCount = "+neighborCount);

            // ok, here's what we gotta do here.
            // we are considering each node as a sender... those are its 
            // "best neighbors" for this purpose, when we're showing all links.
            // we build a hashtable, where keys are src node names, and values
            // are an array of size k.  every time we find a link for a src node,
            // we see if its stats quality is better than any of the current three;
            // if yes, displace the worst.

            Hashtable tmpH = new Hashtable();

            for (int i = 0; i < links.length; ++i) {
                String srcNode = links[i].getSender();
//                if (links[i].getPktPercent() > 0.0f) {
                    GenericLinkStats klinks[] = (GenericLinkStats[])tmpH.get(srcNode);
                    if (klinks == null) {
                        klinks = new GenericLinkStats[this.neighborCount];
                        tmpH.put(srcNode,klinks);
                    }

                    float maxDiff = 0.0f;
                    int maxDiffIdx = -1;
                    int nullIdx = -1;

                    for (int j = 0; j < klinks.length; ++j) {
                        if (klinks[j] != null) {
                            Object lObj = links[i].getStat(this.currentProperty);
                            Object kObj = klinks[j].getStat(this.currentProperty);
                            
                            // this is the only way we can make a meaningful
                            // comparison -- we have no way to take the "diff"
                            // of strings, which are the only other object we allow.
                            if (lObj instanceof Float && kObj instanceof Float) {
                            
                                float diff = ((Float)lObj).floatValue() - ((Float)kObj).floatValue();
                                //System.out.println("computed diff = "+diff);
                                if (diff > 0 && diff > maxDiff) {
                                    maxDiff = diff;
                                    maxDiffIdx = j;
                                }
                            }
                        }
                        else {
                            nullIdx = j;
                            break;
                        }
                    }

                    if (nullIdx != -1) {
                        klinks[nullIdx] = links[i];
                    }
                    else if (maxDiffIdx > -1) {
                        klinks[maxDiffIdx] = links[i];
                    }
                    else {
                        System.out.println("waaah");
                    }


//                }

            }

            int count = 0;
            for (Enumeration e1 = tmpH.elements(); e1.hasMoreElements(); ) {
                GenericLinkStats klinks[] = (GenericLinkStats[])e1.nextElement();
                if (klinks != null) {
                    for (int j = 0; j < klinks.length; ++j) {
                        if (klinks[j] != null) {
                            tmp.add(klinks[j]);
                            ++count;
                        }
                    }
                }
            }
            //System.out.println("added "+count+" from tmpH!");

            retval = new GenericLinkStats[tmp.size()];
            int i = 0;
            for (Enumeration e1 = tmp.elements(); e1.hasMoreElements(); ) {
                retval[i] = (GenericLinkStats)e1.nextElement();
                ++i;
            }
        }
        
        // now try the options...
        Boolean val = (Boolean)options.get(OPTION_NO_ZERO_LINKS);
        if (val != null && val.booleanValue() && retval != null) {
            Vector opt = new Vector();
            for (int i = 0; i < retval.length; ++i) {
                Object lObj = retval[i].getStat(this.currentProperty);
                if ((lObj instanceof Float && ((Float)lObj).floatValue() != 0.0f) ||
                    (lObj instanceof String && !((String)lObj).equals(""))) {
                    // add this one in
                    opt.add(retval[i]);
                }
            }
            
            GenericLinkStats rtmp[] = new GenericLinkStats[opt.size()];
            int i = 0;
            for (Enumeration e1 = opt.elements(); e1.hasMoreElements(); ) {
                rtmp[i] = (GenericLinkStats)e1.nextElement();
                ++i;
            }
            
            retval = rtmp;
        }
        
        return retval;
    }
    
    protected GenericLinkStats[] getCurrentLinks() {
        // this considers our current state and returns the appropriate 
        // linkstats objects to draw
        GenericLinkStats retval[] = null;
        
        if (mode == MODE_ALL) {
            Vector tmp = new Vector();
            GenericLinkStats links[] = data.getAllStatsAtIndexValues(this.currentIndexValues);
            
            if (links == null || links.length == 0) {
                retval = null;
            }
            else {
//                for (int i = 0; i < links.length; ++i) {
//                    tmp.add(links[i]);
//                }
                
                retval = filterAccordingToLimit(links);
            }
        }
        else if (mode == MODE_MST) {
            // compute a minimum spanning tree...
            // using Prim's algorithm and a minHeap.
            
            
            
        }
        else if (mode == MODE_SELECT_SRC) {
            // ok, we have to go through for all nodes and find all their
            // send stats to other receivers
            // fortunately, teh WirelessData iface makes it easy for us...
            // wait, what am I talking about, 'fortunately' ?
            
            Vector tmp = new Vector();
            
            for (Enumeration e1 = this.selectionList.elements(); e1.hasMoreElements(); ) {
                String srcNode = (String)e1.nextElement();
                GenericLinkStats ls[] = data.getSendStatsAtIndexValues(srcNode,this.currentIndexValues);
                if (ls != null) {
                    for (int i = 0; i < ls.length; ++i) {
                        tmp.add(ls[i]);
                    }
                }
            }
            
            retval = new GenericLinkStats[tmp.size()];
            int i = 0;
            for (Enumeration e1 = tmp.elements(); e1.hasMoreElements(); ) {
                retval[i] = (GenericLinkStats)e1.nextElement();
                ++i;
            }
            
            retval = this.filterAccordingToLimit(retval);
            
        }
        else if (mode == MODE_SELECT_DST) {
            Vector tmp = new Vector();
            
            for (Enumeration e1 = this.selectionList.elements(); e1.hasMoreElements(); ) {
                String recvNode = (String)e1.nextElement();
                GenericLinkStats ls[] = data.getRecvStatsAtIndexValues(recvNode,this.currentIndexValues);
                if (ls != null) {
                    for (int i = 0; i < ls.length; ++i) {
                        tmp.add(ls[i]);
                    }
                }
            }
            
            retval = new GenericLinkStats[tmp.size()];
            int i = 0;
            for (Enumeration e1 = tmp.elements(); e1.hasMoreElements(); ) {
                retval[i] = (GenericLinkStats)e1.nextElement();
                ++i;
            }
            
            retval = this.filterAccordingToLimit(retval);
        }
        else {
            // should never get here.
            retval = null;
        }
        
        return retval;
    }
    
    protected Point getPosition(String node) {
        return positions.getPoint(node);
    }
    
    public Dataset getDataset() {
        return this.dataset;
    }
    
    public void addChangeListener(ChangeListener listener) {
        if (listener != null && !changeListeners.contains(listener)) {
            changeListeners.add(listener);
        }
    }
    
    public void removeChangeListener(ChangeListener listener) {
        if (listener != null && changeListeners.contains(listener)) {
            changeListeners.remove(listener);
        }
    }
    
    // accessor methods
    public GenericWirelessData getData() {
        return data;
    }
    
    public String getCurrentProperty() {
        return currentProperty;
    }
    
    public Vector getSelection() {
        return selectionList;
    }
    
    public Object getThreshold() {
        return threshold;
    }
    
    public int getNeighborCount() {
        return neighborCount;
    }
    
    public int getMode() {
        return mode;
    }
    
    public String[] getCurrentIndexValues() {
        return this.currentIndexValues;
    }
    
    public int getLimit() {
        return limit;
    }
    
    public boolean getOption(Integer opt) {
        Boolean b = (Boolean)options.get(opt);
        if (b != null) {
            return b.booleanValue();
        }
        else {
            return false;
        }
    }
    
    // this is a hack to make the NodeMapPanel redraw when we need it to
    void fireChangeListeners() {
        notifyChangeListeners();
    }
    
    public void setFloor(int floor) {
        this.currentFloor = floor;
        notifyChangeListeners();
    }
    
    public void setScale(int scale) {
        this.currentScale = scale;
        notifyChangeListeners();
    }
    
    public int getFloor() {
        return this.currentFloor;
    }
    
    public int getScale() {
        return this.currentScale;
    }
    
    public int getNextScale() {
        if (this.currentScale != this.getMaxScale()) {
            int i = 1;
            while (!this.isValidScale(this.currentScale + i)) {
                ++i;
            }
            return (this.currentScale + i);
        }
        else {
            return this.currentScale;
        }
    }
    
    public int getPrevScale() {
        if (this.currentScale != this.getMinScale()) {
            int i = 1;
            while (!this.isValidScale(this.currentScale - i)) {
                ++i;
            }
            return (this.currentScale - 1);
        }
        else {
            return this.currentScale;
        }
    }
    
    public boolean isValidScale(int s) {
        return this.dataset.isScale(s);
    }
    
    public int getMinScale() {
        return this.minScale;
    }
    
    public int getMaxScale() {
        return this.maxScale;
    }
    
    public float getScaleFactor() {
        return this.dataset.getScaleFactor(this.currentScale);
    }
    
    private void notifyChangeListeners() {
        for (Enumeration e1 = changeListeners.elements(); e1.hasMoreElements(); ) {
            ((ChangeListener)e1.nextElement()).stateChanged(new ChangeEvent(this));
        }
    }
    
    // set stuff up...
    public boolean setOption(Integer option,Boolean val) {
        if (option.intValue() == OPTION_NO_ZERO_LINKS.intValue()) {
            options.put(option,val);
            notifyChangeListeners();
            return true;
        }
        else {
            return false;
        }
    }
    
    public boolean setMode(int mode) {
        if (mode == MODE_ALL || mode == MODE_SELECT_SRC || mode == MODE_SELECT_DST) {
            this.mode = mode;
            notifyChangeListeners();
            return true;
        }
        else {
            return false;
        }
    }
    
    public boolean setLimit(int limit) {
        if (limit == LIMIT_NONE || limit == LIMIT_THRESHOLD || limit == LIMIT_N_BEST_NEIGHBOR) {
            this.limit = limit;
            notifyChangeListeners();
            return true;
        }
        else {
            return false;
        }
    }
    
    // outside parties should not call this for now!
    // just be a good little man and create a new object!
    // fear not the mighty garbage collector!
//    private void setData(GenericWirelessData data) {
//        this.data = data;
//        // reset all mode/properties, to avoid nasty failures.
//        this.selectionList.clear();
//        this.threshold = 0.70f;
//        this.neighborCount = 3;
//        
//        this.mode = MODE_ALL;
//        this.limit = LIMIT_THRESHOLD;
//        
//        notifyChangeListeners();
//    }
    
    public void addNodeToSelection(String node) {
        if (!selectionList.contains(node)) {
            selectionList.add(node);
        }
        notifyChangeListeners();
    }
    
    public void setSelection(Vector selection) {
        this.selectionList = selection;
        notifyChangeListeners();
    }
    
    public void removeNodeFromSelection(String node) {
        if (selectionList.contains(node)) {
            selectionList.remove(node);
            notifyChangeListeners();
        }
    }
    
    public boolean setThreshold(Object threshold) {
//        if (threshold < 0.0f || threshold > 1.0f) {
//            return false;
//        }
//        else {
            this.threshold = threshold;
            notifyChangeListeners();
            return true;
//        }
    }
    
    public boolean setNeighborCount(int count) {
        if (count > 0) {
            this.neighborCount = count;
            notifyChangeListeners();
            
            return true;
        }
        else {
            return false;
        }
    }
    
    public void setIndexValues(String[] indexValues) {
        this.currentIndexValues = indexValues;
        notifyChangeListeners();
    }
    
    public void setCurrentProperty(String property) {
        this.currentProperty = property;
        notifyChangeListeners();
    }
    
}
