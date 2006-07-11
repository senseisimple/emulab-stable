/*
 * GenericLinkStats.java
 *
 * Created on July 4, 2006, 11:08 AM
 *
 * To change this template, choose Tools | Template Manager
 * and open the template in the editor.
 */

import java.util.*;

/**
 *
 * @author david
 */
public class GenericLinkStats {
    
    private GenericStats gs;
    private Hashtable stats;
    private Hashtable metaStats;
    private String recvNode;
    private String sendNode;
    
    /** Creates a new instance of GenericLinkStats */
    public GenericLinkStats(String recvNode,String sendNode) {
        this.gs = null;
        this.stats = new Hashtable();
        this.metaStats = new Hashtable();
        this.recvNode = recvNode;
        this.sendNode = sendNode;
    }
    
    public void addStat(String statName,Object stat) {
        stats.put(statName,stat);
        //System.out.println("GLS: added "+recvNode+" <- "+sendNode+", "+statName+"="+stat);
    }
    
    public void addMetaStat(String statName,String metaName,Object stat) {
        Hashtable mst = (Hashtable)metaStats.get(statName);
        if (mst == null) {
            mst = new Hashtable();
            metaStats.put(statName,mst);
        }
        mst.put(metaName,stat);
    }
    
    public void setParentGenericStats(GenericStats p) {
        this.gs = p;
    }
    
    public Float getPctOfPropertyRange(String statName) {
        Object stat = getStat(statName);
        if (stat != null && stat instanceof Float && gs != null) {
            // since we have the parent, we know the min/max prop values.
            // so, compute the range:
            float min = ((Float)gs.getMinPropertyValue(statName)).floatValue();
            float max = ((Float)gs.getMaxPropertyValue(statName)).floatValue();
            float myValue = ((Float)stat).floatValue();
            
            // units of range per percent:
            float myTicks = 100/(max-min);
            float myPercent = (myValue - min)*myTicks;
            
            //System.out.println("GPOPR: statName="+statName+"min="+min+",max="+max+",myValue="+myValue+",myTicks="+myTicks+"myPercent="+myPercent);
            
            return new Float(myPercent);
        }
        
        return null;
    }
    
    public Hashtable getMetaStats(String statName) {
        return (Hashtable)metaStats.get(statName);
    }
    
    public Object getStat(String statName) {
        return stats.get(statName);
    }
    
    public String getReceiver() {
        return recvNode;
    }
    
    public String getSender() {
        return sendNode;
    }
    
    public String toString() {
        String retval = "GLS.toString: recvNode="+recvNode+",sendNode="+sendNode+"(";
        for (Enumeration e1 = stats.keys(); e1.hasMoreElements(); ) {
            String key = (String)e1.nextElement();
            retval += key+"="+stats.get(key);
            if (e1.hasMoreElements()) {
                retval += ",";
            }
        }
        return retval;
    }
    
}
