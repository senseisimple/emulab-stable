/*
 * MapDataModel.java
 *
 * Created on January 29, 2006, 9:15 PM
 *
 * To change this template, choose Tools | Options and locate the template under
 * the Source Creation and Management node. Right-click the template and choose
 * Open. You can then make changes to the template in the Source Editor.
 */

import java.util.*;
import javax.swing.event.*;
import java.awt.Point;

/**
 *
 * @author david
 */
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
    
    private WirelessData data;
    private Vector selectionList;
    private float threshold;
    private int neighborCount;
    private Vector changeListeners;
    private int mode;
    private int limit;
    private NodePosition positions;
    private java.awt.Image mapImage;
    private int powerLevel;
    
    private Hashtable options;
    
    /** Creates a new instance of MapDataModel */
    private MapDataModel() {
        this(null,null);
    }
    
    public MapDataModel(WirelessData data,NodePosition positions) {
        this.data = data;
        this.positions = positions;
        selectionList = new Vector();
        threshold = 0.70f;
        neighborCount = 3;
        changeListeners = new Vector();
        this.powerLevel = -1;
        
        mode = MODE_ALL;
        limit = LIMIT_NONE;
        this.options = new Hashtable();
        options.put(OPTION_NO_ZERO_LINKS,OPTION_SET);
    }
    
    // this is a nice helpful method for the nodemappanel... so it doesn't have
    // to be aware of what kind of functionality this model exposes.
    protected String[] getNodes() {
        return data.getNodes();
    }
    
    private LinkStats[] filterAccordingToLimit(LinkStats links[]) {
        LinkStats retval[] = null;
        Vector tmp = new Vector();
        
        if (limit == LIMIT_NONE) {
            // they all pass...
            retval = links;
        }
        else if (limit == LIMIT_THRESHOLD) {
            // process out the chaff according to the limit
            for (int i = 0; i < links.length; ++i) {
                if (links[i].getPktPercent() >= this.threshold) {
                    tmp.add(links[i]);
                }
            }
            // dump back to array...
            retval = new LinkStats[tmp.size()];
            int i = 0;
            for (Enumeration e1 = tmp.elements(); e1.hasMoreElements(); ) {
                retval[i] = (LinkStats)e1.nextElement();
                ++i;
            }
        }
        else if (limit == LIMIT_N_BEST_NEIGHBOR) {
            retval = null;

            System.out.println("neighborCount = "+neighborCount);

            // ok, here's what we gotta do here.
            // we are considering each node as a sender... those are its 
            // "best neighbors" for this purpose, when we're showing all links.
            // we build a hashtable, where keys are src node names, and values
            // are an array of size k.  every time we find a link for a src node,
            // we see if its stats quality is better than any of the current three;
            // if yes, displace the worst.

            Hashtable tmpH = new Hashtable();

            for (int i = 0; i < links.length; ++i) {
                String srcNode = links[i].getSrcNode();
                if (links[i].getPktPercent() > 0.0f) {
                    LinkStats klinks[] = (LinkStats[])tmpH.get(srcNode);
                    if (klinks == null) {
                        klinks = new LinkStats[this.neighborCount];
                        tmpH.put(srcNode,klinks);
                    }

                    float maxDiff = 0.0f;
                    int maxDiffIdx = -1;
                    int nullIdx = -1;

                    for (int j = 0; j < klinks.length; ++j) {
                        if (klinks[j] != null) {
                            float diff = Math.abs(links[i].getPktPercent() - klinks[j].getPktPercent());
                            if (links[i].getPktPercent() > klinks[j].getPktPercent() && diff > maxDiff) {
                                maxDiff = diff;
                                maxDiffIdx = j;
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


                }

            }

            int count = 0;
            for (Enumeration e1 = tmpH.elements(); e1.hasMoreElements(); ) {
                LinkStats klinks[] = (LinkStats[])e1.nextElement();
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

            retval = new LinkStats[tmp.size()];
            int i = 0;
            for (Enumeration e1 = tmp.elements(); e1.hasMoreElements(); ) {
                retval[i] = (LinkStats)e1.nextElement();
                ++i;
            }
        }
        else {
            retval = null;
        }
        
        // now try the options...
        Boolean val = (Boolean)options.get(OPTION_NO_ZERO_LINKS);
        if (val != null && val.booleanValue() && retval != null) {
            Vector opt = new Vector();
            for (int i = 0; i < retval.length; ++i) {
                if (retval[i].getPktPercent() > 0) {
                    // add this one in
                    opt.add(retval[i]);
                }
            }
            
            LinkStats rtmp[] = new LinkStats[opt.size()];
            int i = 0;
            for (Enumeration e1 = opt.elements(); e1.hasMoreElements(); ) {
                rtmp[i] = (LinkStats)e1.nextElement();
                ++i;
            }
            
            retval = rtmp;
        }
        
        return retval;
    }
    
    protected LinkStats[] getCurrentLinks() {
        // this considers our current state and returns the appropriate 
        // linkstats objects to draw
        LinkStats retval[] = null;
        
        if (mode == MODE_ALL) {
            Vector tmp = new Vector();
            LinkStats links[] = data.getAllStatsAtPowerLevel(this.powerLevel);
            
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
                LinkStats ls[] = (LinkStats[])data.getSendStatsAtPowerLevel(srcNode,this.powerLevel);
                for (int i = 0; i < ls.length; ++i) {
                    tmp.add(ls[i]);
                }
            }
            
            retval = new LinkStats[tmp.size()];
            int i = 0;
            for (Enumeration e1 = tmp.elements(); e1.hasMoreElements(); ) {
                retval[i] = (LinkStats)e1.nextElement();
                ++i;
            }
            
            retval = this.filterAccordingToLimit(retval);
            
        }
        else if (mode == MODE_SELECT_DST) {
            Vector tmp = new Vector();
            
            for (Enumeration e1 = this.selectionList.elements(); e1.hasMoreElements(); ) {
                String recvNode = (String)e1.nextElement();
                LinkStats ls[] = (LinkStats[])data.getRecvStatsAtPowerLevel(recvNode,this.powerLevel);
                for (int i = 0; i < ls.length; ++i) {
                    tmp.add(ls[i]);
                }
            }
            
            retval = new LinkStats[tmp.size()];
            int i = 0;
            for (Enumeration e1 = tmp.elements(); e1.hasMoreElements(); ) {
                retval[i] = (LinkStats)e1.nextElement();
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
    public WirelessData getData() {
        return data;
    }
    
    public Vector getSelection() {
        return selectionList;
    }
    
    public float getThreshold() {
        return threshold;
    }
    
    public int getNeighborCount() {
        return neighborCount;
    }
    
    public int getMode() {
        return mode;
    }
    
    public int getPowerLevel() {
        return powerLevel;
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
    private void setData(WirelessData data) {
        this.data = data;
        // reset all mode/properties, to avoid nasty failures.
        this.selectionList.clear();
        this.threshold = 0.70f;
        this.neighborCount = 3;
        
        this.mode = MODE_ALL;
        this.limit = LIMIT_THRESHOLD;
        
        notifyChangeListeners();
    }
    
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
    
    public boolean setThreshold(float threshold) {
        if (threshold < 0.0f || threshold > 1.0f) {
            return false;
        }
        else {
            this.threshold = threshold;
            notifyChangeListeners();
            return true;
        }
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
    
    public void setPowerLevel(int powerLevel) {
        this.powerLevel = powerLevel;
        notifyChangeListeners();
    }
}
