/*
 * GenericStats.java
 *
 * Created on July 3, 2006, 8:37 PM
 *
 * To change this template, choose Tools | Template Manager
 * and open the template in the editor.
 */

import java.util.*;
import java.io.*;
import java.util.regex.*;

/**
 *
 * @author david
 */
public class GenericStats implements GenericWirelessData {
    
    Hashtable indexMapping;
    String[] indices;
    Vector nodes;
    Vector properties;
    Hashtable minPropValues;
    Hashtable maxPropValues;
    Hashtable indexValues;
    
    /** Creates a new instance of GenericStats */
    protected GenericStats(String[] indices,Hashtable indexValues,
                           Vector nodes,Vector properties,
                           Hashtable minPropValues,Hashtable maxPropValues,
                           Hashtable mapping) {
        this.indices = indices;
        this.indexMapping = mapping;
        this.nodes = nodes;
        this.properties = properties;
        this.minPropValues = minPropValues;
        this.maxPropValues = maxPropValues;
        this.indexValues = indexValues;
    }
    
    public GenericLinkStats[] getSendStatsAtIndexValues(String sendNode,String[] indexValues) {
        String dsString = "";
        for (int i = 0; i < indexValues.length; ++i) {
            dsString += indexValues[i];
            if ((i+1) != indexValues.length) {
                dsString += ",";
            }
        }
        
        Dataset ds = (Dataset)this.indexMapping.get(dsString);
        
        if (ds != null) {
            return ds.getSendStats(sendNode);
        }
        return null;
    }
    
    public GenericLinkStats[] getRecvStatsAtIndexValues(String recvNode,String[] indexValues) {
        String dsString = "";
        for (int i = 0; i < indexValues.length; ++i) {
            dsString += indexValues[i];
            if ((i+1) != indexValues.length) {
                dsString += ",";
            }
        }
        
        Dataset ds = (Dataset)this.indexMapping.get(dsString);
        
        if (ds != null) {
            return ds.getRecvStats(recvNode);
        }
        return null;
    }
    
    public GenericLinkStats[] getAllStatsAtIndexValues(String[] indexValues) {
        String dsString = "";
        for (int i = 0; i < indexValues.length; ++i) {
            dsString += indexValues[i];
            if ((i+1) != indexValues.length) {
                dsString += ",";
            }
        }
        
        System.out.println("trying dsString='"+dsString+"'");
        Dataset ds = (Dataset)this.indexMapping.get(dsString);
        
        if (ds != null) {
            GenericLinkStats[] retval = ds.getAllStats();
            System.out.println("allstats.length = " + retval.length);
            return retval;
        }
        return null;
    }
    
    public GenericLinkStats getStats(String[] indexValues,String recvNode,String sendNode) {
//        Hashtable clh = this.indexMapping;
//        Dataset ds = null;
//        for (int i = 0; i < indexValues.length; ++i) {
//            if ((i+1) == indexValues.length) {
//                ds = (Dataset)clh.get(indexValues[i]);
//            }
//            else {
//                Hashtable tmp = (Hashtable)clh.get(indexValues[i]);
//                if (tmp == null) {
//                    return null;
//                }
//                clh = tmp;
//            }
//        }
        String dsString = "";
        for (int i = 0; i < indexValues.length; ++i) {
            dsString += indexValues[i];
            if ((i+1) != indexValues.length) {
                dsString += ",";
            }
        }
        Dataset ds = (Dataset)this.indexMapping.get(dsString);
        
        if (ds != null) {
            return ds.getStats(recvNode,sendNode);
        }
        
        return null;
    }
    
    
    public Vector getIndexValues(String index) {
        return (Vector)this.indexValues.get(index);
    }
    
    public Vector getProperties() {
        return this.properties;
    }
    
    public Object getMaxPropertyValue(String property) {
        return maxPropValues.get(property);
    }
    
    public Object getMinPropertyValue(String property) {
        return minPropValues.get(property);
    }
    
    public String[] getIndices() {
        return indices;
    }
    
    public String[] getNodes() {
        String[] retval = new String[this.nodes.size()];
        int i = 0;
        for (Enumeration e1 = this.nodes.elements(); e1.hasMoreElements(); ) {
            retval[i] = (String)e1.nextElement();
            ++i;
        }
        return retval;
    }
    
    
    static GenericStats parseInputStream(InputStream is) 
        throws java.io.IOException, Exception {
        BufferedReader br = null;
        
        br = new BufferedReader(new InputStreamReader(is));
        
        return parseReader(br);
    }
    
    static GenericStats parseDumpFile(String filename) 
        throws java.io.IOException, java.text.ParseException,
        java.lang.IllegalArgumentException, Exception {
        File f = null;
        BufferedReader br = null;

        if (filename != null) {
            f = new File(filename);
            if (!f.exists() || !f.canRead()) {
                 throw new java.io.IOException("Couldn't open and/or read " + 
                                               "file "+filename);
            }
        }
        else {
            throw new java.lang.IllegalArgumentException("filename null");
        }
        
        GenericStats retval = null;
        
        try {
            br = new BufferedReader(new FileReader(f));
            retval = parseReader(br);
        }
        catch (Exception ex) {
            ex.printStackTrace();
        }
        
        return retval;
    }
    
    private static GenericStats parseReader(BufferedReader br)
        throws IOException {
        
        Hashtable mapping = new Hashtable();
        Vector nodes = new Vector();
        String[] indices = null;
        Vector properties = new Vector();
        //
        // Unfortunately, it only makes sense to figure these across the whole 
        // file (dataset).  They're only used to generate the link colorings, so
        // it's important that the color scale look the same for the entire 
        // dataset (file).  That's my opinion, anyway... plus, it's the easy 
        // thing to do!
        //
        Hashtable maxPropValues = new Hashtable();
        Hashtable minPropValues = new Hashtable();
        Hashtable indexValues = new Hashtable();
        
        int linecount = 0;
        
        try {
            String line = null;
            
            
            
            // parse line: "INDICES(indexname,indexname2,...,indexnamen)""
            Pattern nodePattern = Pattern.compile("\\s*(.+)\\s+<\\-\\s+(.+)");
            Pattern singleDataPattern = Pattern.compile("\\s*(.+)=(.+)");
            Pattern statsDataPattern = Pattern.compile("\\s*(.+)=(.+)\\s+\\((.+)\\)");
                        
            boolean inDataset = false;
            Dataset dataset = null;
            boolean haveIndices = false;
            String[] currentIndexValues = null;
            
            while ((line = br.readLine()) != null) {
                ++linecount;
                
                if (line.startsWith("INDICES")) {
                    if (haveIndices) {
                        System.err.println("line " + linecount + ": already have indices!");
                        return null;
                    }
                    line = line.replaceFirst("INDICES\\s*","");
                    
                    String[] sa = line.split("\\,");
                    indices = new String[sa.length];
                    
                    for (int i = 0; i < sa.length; ++i) {
                        // get rid of leading whitespace...
                        String iName = sa[i].replaceAll("^\\s*","");
                        // get rid of trailing whitespace...
                        iName = iName.replaceAll("\\s*$","");
                        
                        indices[i] = iName;
                    }
                    
                    haveIndices = true;
                }
                else if (line.startsWith("STARTDATA")) {
                    if (!haveIndices) {
                        System.err.println("line " + linecount + ": no indices specified!");
                        return null;
                    }
                    
                    if (inDataset) {
                        System.err.println("line " + linecount + ": STARTDATA, but no preceding ENDDATA!");
                        return null;
                    }
                    else {
                        line = line.replaceFirst("STARTDATA\\s*","");
                        
                        String[] sa = line.split("\\,");
                        
                        if (sa.length != indices.length) {
                            System.err.println("line " + linecount + ": different number of index values than indices!");
                            return null;
                        }
                        
                        for (int i = 0; i < sa.length; ++i) {
                            // get rid of leading whitespace...
                            String iName = sa[i].replaceAll("^\\s*","");
                            // get rid of trailing whitespace...
                            iName = iName.replaceAll("\\s*$","");
                            // leave all these as strings...
                            sa[i] = iName;
                            
                        }
                        
                        // stick in the current set...
                        currentIndexValues = sa;
                        String dsString = "";
                        for (int i = 0; i < currentIndexValues.length; ++i) {
                            dsString += currentIndexValues[i];
                            if ((i+1) != currentIndexValues.length) {
                                dsString += ",";
                            }
                            
                            // also add to the vector in the indexValues hash for 
                            // this index:
                            Vector tv = (Vector)indexValues.get(indices[i]);
                            if (tv == null) {
                                tv = new Vector();
                                indexValues.put(indices[i],tv);
                            }
                            if (!tv.contains(currentIndexValues[i])) {
                                tv.add(currentIndexValues[i]);
                            }
                        }
                        dataset = (Dataset)mapping.get(dsString);
                        if (dataset == null) {
                            dataset = new Dataset();
                            mapping.put(dsString,dataset);
                        }
                        inDataset = true;
                        // based on currentIndexValues, add to the main hash:
                        
                        
//                        Dataset clh = (Hashtable)mapping.get(dsString);
//                        if (clh == null) {
//                            clh = new Hashtable
//                        }
//                        for (int i = 0; i < currentIndexValues.length; ++i) {
//                            dsString += currentIndexValues[i];
//                            if ((i+1) != currentIndexValues.length) {
//                                dsString += ",";
//                            }
//                            if ((i+1) == currentIndexValues.length) {
//                                // i.e., if we're on the last index value, we don't want to 
//                                // add a new hashtable just for this single dataset...
//                                clh.put(sa[i],dataset);
//                            }
//                            else {
//                                Hashtable mth = (Hashtable)clh.get(sa[i]);
//                                if (mth == null) {
//                                    mth = new Hashtable();
//                                    clh.put(sa[i],mth);
//                                }
//                                clh = mth;
//                            }
//                        }
                        
                    }
                }
                else if (line.startsWith("ENDDATA")) {
                    if (!inDataset) {
                        System.err.println("line " + linecount + ": ENDDATA, but no STARTDATA!");
                    }
                    else {
                        dataset = null;
                        currentIndexValues = null;
                        inDataset = false;
                    }
                }
                else if (line == null || line.equals("") 
                         || line.startsWith("#") || line.startsWith("//")) {
                    // comment, whitespace...
                }
                else {
                    // this should be a line.
                    String[] sa = line.split(":");
                    if (sa.length != 2) {
                        System.err.println("line " + linecount + ": not even data line!");
                    }
                    else {
                        //System.err.println("nodepatternline='" + sa[0] + "'");
                        
                        // see if the node <- node pattern matches:
                        Matcher m = nodePattern.matcher(sa[0]);
                        String sendNode,recvNode;
                        if (m.matches()) {
                            recvNode = m.group(1);
                            sendNode = m.group(2);
                            
                            GenericLinkStats gls = new GenericLinkStats(recvNode,sendNode);
                            dataset.addReceiverStats(recvNode,sendNode,gls);
                            
                            // add to nodes list if necessary:
                            if (!nodes.contains(recvNode)) {
                                nodes.add(recvNode);
                            }
                            if (!nodes.contains(sendNode)) {
                                nodes.add(sendNode);
                            }
                            
                            // now check the various properties:
                            String[] ta = sa[1].split("\\s*\\;\\s*");
                            for (int i = 0; i < ta.length; ++i) {
                                if (ta[i] != null && !ta[i].equals("")) {
                                    //System.err.println("ta["+i+"]='"+ta[i]+"'");
                                    
                                    // check and see if value is "0.0 (0.0,...0.0)"
                                    // or just "0.0"
                                    Matcher dm = singleDataPattern.matcher(ta[i]);
                                    Matcher sdm = statsDataPattern.matcher(ta[i]);
                                    if (sdm.matches()) {
                                        // try float first:
                                        String statName = sdm.group(1);
                                        
                                        //System.err.println("statName='"+statName+"'");
                                        
                                        if (!properties.contains(statName)) {
                                            properties.add(statName);
                                        }
                                        
                                        Object obj = null;
                                        try {
                                            // so at least grabbing it as a string will work.
                                            obj = sdm.group(1);
                                            // try to take as float if possible...
                                            obj = new Float(Float.parseFloat(sdm.group(2)));
                                        }
                                        catch (Exception ex) {
                                            ;
                                        }
                                        
                                        // figure out if this value is less that the current minPropValue,
                                        // or greater thatn the current maxPropValue:
                                        Object cmm = minPropValues.get(statName);
                                        if (cmm == null && obj instanceof Comparable) {
                                            minPropValues.put(statName,obj);
                                        }
                                        else {
                                            // test for less than, icky...
                                            if (obj instanceof Comparable && cmm.getClass() == obj.getClass()) {
                                                if (((Comparable)obj).compareTo((Comparable)cmm) < 0) {
                                                    minPropValues.put(statName,obj);
                                                }
                                            }
                                        }
                                        cmm = maxPropValues.get(statName);
                                        if (cmm == null && obj instanceof Comparable) {
                                            maxPropValues.put(statName,obj);
                                        }
                                        else {
                                            // test for greater than, icky...
                                            if (obj instanceof Comparable && cmm.getClass() == obj.getClass()) {
                                                if (((Comparable)obj).compareTo((Comparable)cmm) > 0) {
                                                    maxPropValues.put(statName,obj);
                                                }
                                            }
                                        }
                                        
                                        // add the stat
                                        gls.addStat(statName,obj);
                                        
                                        // gotta parse the stats bit too...
                                        String[] msa = sdm.group(3).split("\\,");
                                        for (int j = 0; j < msa.length; ++j) {
                                            String[] tmsa = msa[i].split("=");
                                            if (tmsa.length == 2) {
                                                String msName = tmsa[0];
                                                Object msObj = tmsa[1];
                                                try {
                                                    msObj = new Float(Float.parseFloat(tmsa[1]));
                                                }
                                                catch (Exception ex) {
                                                    ;
                                                }
                                                
                                                gls.addMetaStat(statName,msName,msObj);
                                            }
                                            else {
                                                System.err.println("line " + linecount + ": bad data metastat!");
                                            }
                                        }
                                        
                                    }
                                    else if (dm.matches()) {
                                        // try float first:
                                        String statName = dm.group(1);
                                        
                                        //System.err.println("statName='"+statName+"'");
                                        
                                        if (!properties.contains(statName)) {
                                            properties.add(statName);
                                        }
                                        
                                        Object obj = null;
                                        try {
                                            // so at least grabbing it as a string will work.
                                            obj = dm.group(1);
                                            // try to take as float if possible...
                                            obj = new Float(Float.parseFloat(dm.group(2)));
                                        }
                                        catch (Exception ex) {
                                            ;
                                        }
                                        
                                        // figure out if this value is less that the current minPropValue,
                                        // or greater thatn the current maxPropValue:
                                        Object cmm = minPropValues.get(statName);
                                        if (cmm == null && obj instanceof Comparable) {
                                            minPropValues.put(statName,obj);
                                        }
                                        else {
                                            // test for less than, icky...
                                            if (obj instanceof Comparable && cmm.getClass() == obj.getClass()) {
                                                if (((Comparable)obj).compareTo((Comparable)cmm) < 0) {
                                                    minPropValues.put(statName,obj);
                                                }
                                            }
                                        }
                                        cmm = maxPropValues.get(statName);
                                        if (cmm == null && obj instanceof Comparable) {
                                            maxPropValues.put(statName,obj);
                                        }
                                        else {
                                            // test for greater than, icky...
                                            if (obj instanceof Comparable && cmm.getClass() == obj.getClass()) {
                                                if (((Comparable)obj).compareTo((Comparable)cmm) > 0) {
                                                    maxPropValues.put(statName,obj);
                                                }
                                            }
                                        }
                                        
                                        // add the stat
                                        gls.addStat(statName,obj);
                                    }
                                    else {
                                        System.out.println("line " + linecount + ": bad data line!");
                                    }
                                }
                            }
                        }
                        else {
                            System.err.println("line " + linecount + ": malformed recv <- send bits");
                        }
                    }
                }
                
            }
            
        }
        catch (IOException e) {
            throw new IOException("file read failed");
        }
        
        System.out.println("GenericStats parsed "+linecount+" lines.");
        
        return new GenericStats(indices,indexValues,
                                nodes,properties,
                                minPropValues,maxPropValues,
                                mapping);
    }
    
    
    static class Dataset {
        Hashtable recv;
        Hashtable send;
        Vector allStats;
        
        public Dataset() {
            this.recv = new Hashtable();
            this.send = new Hashtable();
            // another optimization...
            this.allStats = new Vector();
        }
        
        public void addReceiverStats(String recvNode,String sendNode,GenericLinkStats gls) {
            Hashtable senders = (Hashtable)recv.get(recvNode);
            if (senders == null) {
                senders = new Hashtable();
                recv.put(recvNode,senders);
            }
            senders.put(sendNode,gls);
            
            // also add it in the reverse direction to the send hash:
            // (optimization for the getSendStats function...)
            Hashtable receivers = (Hashtable)send.get(sendNode);
            if (receivers == null) {
                receivers = new Hashtable();
                send.put(sendNode,receivers);
            }
            receivers.put(recvNode,gls);
            
            // the "allStats" optimization
            //if (!allStats.contains(gls)) {
                // assume that duplicates don't occur... they wouldn't 
                // hurt anyway!
                allStats.add(gls);
            //}
                
            System.out.println("GS.addReceiverStats: added '"+gls.toString()+"'!");
        }
        
        public GenericLinkStats getStats(String recvNode,String sendNode) {
            Hashtable senders = (Hashtable)recv.get(recvNode);
            
            if (senders != null) {
                return (GenericLinkStats)senders.get(sendNode);
            }
            
            return null;
        }
        
        public GenericLinkStats[] getRecvStats(String recvNode) {
            Hashtable senders = (Hashtable)recv.get(recvNode);
            if (senders == null) {
                return null;
            }
            GenericLinkStats[] retval = new GenericLinkStats[senders.size()];
            int i = 0;
            for (Enumeration e1 = senders.keys(); e1.hasMoreElements(); ) {
                String key = (String)e1.nextElement();
                retval[i++] = (GenericLinkStats)senders.get(key);
            }
            
            return retval;
        }
        
        public GenericLinkStats[] getSendStats(String sendNode) {
            Hashtable receivers = (Hashtable)send.get(sendNode);
            if (receivers == null) {
                return null;
            }
            GenericLinkStats[] retval = new GenericLinkStats[receivers.size()];
            int i = 0;
            for (Enumeration e1 = receivers.keys(); e1.hasMoreElements(); ) {
                String key = (String)e1.nextElement();
                retval[i++] = (GenericLinkStats)receivers.get(key);
            }
            
            return retval;
        }
        
        public GenericLinkStats[] getAllStats() {
            System.out.println("real allStats.size in ds ="+allStats.size());
            GenericLinkStats[] retval = new GenericLinkStats[allStats.size()];
            int i = 0;
            for (Enumeration e1 = allStats.elements(); e1.hasMoreElements(); ) {
               retval[i++] = (GenericLinkStats)e1.nextElement();
            }
            
            return retval;
        }
    }
    
    public static void main(String[] args) throws Exception {
        BufferedReader br = new BufferedReader(new FileReader("/home/david/from_cvs/wireless-stats/nn_client.log"));
        GenericStats gs = GenericStats.parseReader(br);
        
        String[] idx = gs.getIndices();
        System.out.print("indices: ");
        for (int i = 0; i < idx.length; ++i) {
            System.out.print("'"+idx[i]+"'");
            
            // check the possible index values:
            Vector iv = gs.getIndexValues(idx[i]);
            System.out.print(" (");
            if (iv != null) {
                for (Enumeration e1 = iv.elements(); e1.hasMoreElements(); ) {
                    System.out.print(""+e1.nextElement());
                    if (e1.hasMoreElements()) {
                        System.out.print(",");
                    }
                }
            }
            System.out.print(")");
            
            if ((i+1) != idx.length) {
                System.out.print(",");
            }
        }
        System.out.println();
        
        String[] nodes = gs.getNodes();
        System.out.print("nodes: ");
        for (int i = 0; i < nodes.length; ++i) {
            System.out.print(""+nodes[i]);
            if ((i+1) != nodes.length) {
                System.out.print(",");
            }
        }
        System.out.println();
        
        Vector p = gs.getProperties();
        System.out.print("properties: ");
        for (Enumeration e1 = p.elements(); e1.hasMoreElements(); ) {
            String prop = (String)e1.nextElement();
            String mv = "(min="+gs.getMinPropertyValue(prop)+",max="+gs.getMaxPropertyValue(prop)+")";
            System.out.print(""+prop+""+mv);
            
            if (e1.hasMoreElements()) {
                System.out.print(",");
            }
        }
        System.out.println();
        
    }
    
}
