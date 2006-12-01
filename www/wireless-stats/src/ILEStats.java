/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005-2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

import java.util.*;
import java.io.*;
import java.util.regex.*;

public class ILEStats implements WirelessData {
    /* strings of format 'moteXXX' */
    private Vector moteNames;
    /* a hashtable of powerLevels->(receivers->(senders,Stats))
     * expected,percent,rssi(min,max,avg,stddev))
     */
    public Hashtable packetStats;
    
    private ILEStats(Vector moteNames,Hashtable packetStats) {
        this.moteNames = moteNames;
        this.packetStats = packetStats;
    }
    
    static ILEStats parseDB() {
        return null;
    }
    
    private static ILEStats parseReader(BufferedReader br) throws Exception {
        Hashtable stats = new Hashtable();
        Vector moteNames = new Vector();
        //Vector moteIDs = new Vector();
        
        int linecount = 0;
        
        try {
            //br = new BufferedReader(new FileReader(f));
            
            String line = null;
            int powerLevel = -1;
            Hashtable pLHash = null;
            
            while ((line = br.readLine()) != null) {
                ++linecount;
                // parse line: "added phs for mote id 102"
                Pattern mote = Pattern.compile("added phs for mote id (\\d+)");
                Matcher moteM = mote.matcher(line);
                // parse line: "Data for power level 0x2"
                Pattern power = Pattern.compile("Data for power level 0x" + 
						"([0-9a-fA-F]+)");
                Matcher powerM = power.matcher(line);
                // parse line: "(114) <- (123): pkts(0/100,0.0%) "
		//    "rssi(0.0,0.0,0.0,0.0)"
                String dataPattern = "\\((\\d+)\\)\\s<\\-\\s\\((\\d+)\\):" + 
		    "\\spkts\\((\\d+)/(\\d+)\\,(\\-*\\d+\\.\\d+E*\\-*\\d*)\\" +
		    "%\\)\\srssi\\((\\-*\\d+\\.\\d+E*\\-*\\d*)\\," + 
		    "(\\-*\\d+\\.\\d+E*\\-*\\d*),(\\-*\\d+\\.\\d+E*\\-*\\d*),"+
		    "(\\-*\\d+\\.\\d+E*\\-*\\d*)\\)";
                Pattern data = Pattern.compile(dataPattern);
                Matcher dataM = data.matcher(line);
                
                if (moteM.matches()) {
                    Integer i = new Integer(Integer.parseInt(moteM.group(1)));
                    //moteIDs.add(i);
                    if (!moteNames.contains("mote"+moteM.group(1))) {
                        moteNames.add("mote"+moteM.group(1));
                    }
                }
                else if (powerM.matches()) {
                    if (pLHash != null) {
                        stats.put(new Integer(powerLevel),pLHash);
                    }
                    powerLevel = Integer.parseInt(powerM.group(1),16);
                    pLHash = new Hashtable();
                }
                else if (dataM.matches()) {
                    int recv = Integer.parseInt(dataM.group(1));
                    int send = Integer.parseInt(dataM.group(2));
                    int pktRecvCount = Integer.parseInt(dataM.group(3));
                    int pktCount = Integer.parseInt(dataM.group(4));
                    float pktPercent = Float.parseFloat(dataM.group(5));
                    float minRSSI = Float.parseFloat(dataM.group(6));
                    float maxRSSI = Float.parseFloat(dataM.group(7));
                    float avgRSSI = Float.parseFloat(dataM.group(8));
                    float stddevRSSI = Float.parseFloat(dataM.group(9));
                    
//                    System.out.println("("+recv+") <- ("+send+"): pkts(" + 
//				       pktRecvCount+"/"+pktCount+"," + 
//				       pktPercent+"%) rssi("+minRSSI+"," + 
//				       maxRSSI+","+avgRSSI+","+stddevRSSI+")");
                    
                    Hashtable r = null;
                    String rNode = "mote"+recv;
                    String sNode = "mote"+send;
                    
                    if ((r = (Hashtable)(pLHash.get(rNode))) == null) {
                        r = new Hashtable();
                        pLHash.put(rNode,r);
                    }
                    
                    r.put(sNode,new LinkStats(sNode,rNode,
                                              pktRecvCount,pktCount,minRSSI,
                                              maxRSSI,avgRSSI,stddevRSSI));
                }
                else {
                    // do nothing
                }
                
            }
            
        }
        catch (IOException e) {
            throw new Exception("file read failed");
        }
        
        System.out.println("ILEStats parsed "+linecount+" lines.");
        
        return new ILEStats(moteNames,stats);
    }
    
    static ILEStats parseInputStream(InputStream is) 
        throws java.io.IOException, Exception {
        BufferedReader br = null;
        
        br = new BufferedReader(new InputStreamReader(is));
        
        return parseReader(br);
    }
    
    static ILEStats parseDumpFile(String filename) 
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
        
        ILEStats retval = null;
        
        try {
            br = new BufferedReader(new FileReader(f));
            retval = parseReader(br);
        }
        catch (Exception ex) {
            ex.printStackTrace();
        }
        
        return retval;
    }
    
    
    // implement WirelessData
    public String[] getNodes() {
        Vector nodes = new Vector();
        for (Enumeration e1 = this.packetStats.keys(); e1.hasMoreElements(); ) {
            Hashtable hashA = (Hashtable)this.packetStats.get(e1.nextElement());
            for (Enumeration e2 = hashA.keys(); e2.hasMoreElements(); ) {
                String nodeA = (String)e2.nextElement();
                if (!nodes.contains(nodeA)) {
                    nodes.add(nodeA);
                }
                Hashtable hashB = (Hashtable)hashA.get(nodeA);
                for (Enumeration e3 = hashB.keys(); e3.hasMoreElements(); ) {
                    String nodeB = (String)e3.nextElement();
                    if (!nodes.contains(nodeB)) {
                        nodes.add(nodeB);
                    }
                }
            }
        }
        
        String retval[] = new String[nodes.size()];
        int i = 0;
        for (Enumeration e1 = nodes.elements(); e1.hasMoreElements(); ) {
            String elm = (String)e1.nextElement();
            retval[i] = elm;
            ++i;
        }
        //String retval[] = (String[])nodes.toArray();
        
        return retval;
    }
    
    public int[] getPowerLevels() {
        int retval[] = new int[this.packetStats.size()];
        int cnt = 0;
        
        for (Enumeration e1 = this.packetStats.keys(); e1.hasMoreElements(); ) {
            retval[cnt] = ((Integer)e1.nextElement()).intValue();
            ++cnt;
        }
        
        return retval;
    }

    // otherwise you have to use these... ick!
    public LinkStats[] getSendStatsAtPowerLevel(String src,int powerLevel) {
        Integer pL = new Integer(powerLevel);
        
        if (this.packetStats.get(pL) == null) {
            return null;
        }
        
        Vector stats = new Vector();
        
        Hashtable recvHash = (Hashtable)this.packetStats.get(pL);
        for (Enumeration e1 = recvHash.keys(); e1.hasMoreElements(); ) {
            String recvNode = (String)e1.nextElement();
        
            Hashtable srcHash = (Hashtable)recvHash.get(recvNode);
            for (Enumeration e2 = srcHash.keys(); e2.hasMoreElements(); ) {
                String srcNode = (String)e2.nextElement();
                LinkStats ls = (LinkStats)srcHash.get(srcNode);
                
                if (srcNode.equals(src) && !stats.contains(ls)) {
                    stats.add(ls);
                }
            }
        }
        
        LinkStats retval[] = new LinkStats[stats.size()];
        int cnt = 0;
        
        for (Enumeration e1 = stats.elements(); e1.hasMoreElements(); ) {
            retval[cnt] = (LinkStats)e1.nextElement();
            ++cnt;
        }
        
        return retval;
    }
    
    public LinkStats[] getRecvStatsAtPowerLevel(String recv,int powerLevel) {
        Integer pL = new Integer(powerLevel);
        
        if (this.packetStats.get(pL) == null) {
            return null;
        }
        
        Vector stats = new Vector();
        
        Hashtable recvHash = (Hashtable)this.packetStats.get(pL);
        for (Enumeration e1 = recvHash.keys(); e1.hasMoreElements(); ) {
            String recvNode = (String)e1.nextElement();
            
            if (recvNode.equals(recv)) {
                Hashtable srcHash = (Hashtable)recvHash.get(recvNode);
                
                for (Enumeration e2 = srcHash.elements(); e2.hasMoreElements(); ) {
                    stats.add(e2.nextElement());
                }
                
                break;
            }
        }
        
        LinkStats retval[] = new LinkStats[stats.size()];
        int cnt = 0;
        
        for (Enumeration e1 = stats.elements(); e1.hasMoreElements(); ) {
            retval[cnt] = (LinkStats)e1.nextElement();
            ++cnt;
        }
        
        return retval;
    }
    
    public LinkStats[] getAllStatsAtPowerLevel(int powerLevel) {
        Vector stats = new Vector();
        
        Hashtable recvHash = null;
        
        if ((recvHash = (Hashtable)this.packetStats.get(new Integer(powerLevel))) == null) {
            return null;
        }
        
        for (Enumeration e1 = recvHash.keys(); e1.hasMoreElements(); ) {
            String recvNode = (String)e1.nextElement();
            Hashtable srcHash = (Hashtable)recvHash.get(recvNode);
            
            for (Enumeration e2 = srcHash.keys(); e2.hasMoreElements(); ) {
                String srcNode = (String)e2.nextElement();
                LinkStats ls = (LinkStats)srcHash.get(srcNode);
                stats.add(ls);
            }
        }
        
        LinkStats retval[] = new LinkStats[stats.size()];
        int i = 0;
        for (Enumeration e1 = stats.elements(); e1.hasMoreElements(); ) {
            Object obj = e1.nextElement();
            //System.out.println("obj = "+obj);
            retval[i] = (LinkStats)obj;
            ++i;
        }
        
        return retval;
    }

    
    
    
    public static void main(String args[]) throws Exception {
        parseDumpFile(args[0]);
    }
    
    // old accessor methods...
    
    public Vector getMoteList() {
        return this.moteNames;
    }
    
    public Vector getRecvMotes() {
        Vector retval = new Vector();
        
        for (Enumeration e1 = this.packetStats.keys(); e1.hasMoreElements(); ) {
            Hashtable recv = (Hashtable)(this.packetStats.get(e1.nextElement()));
            if (recv != null) {
                for (Enumeration e2 = recv.keys(); e2.hasMoreElements(); ) {
                    Object elm = e2.nextElement();
                    if (!retval.contains(elm) && elm != null) {
                        retval.add(elm);
                    }
                }
            }
        }
        
        return retval;
    }
    
    public Vector getRecvMotesAtPower(int powerLevel) {
        Vector retval = new Vector();
        Hashtable h = (Hashtable)(this.packetStats.get(new Integer(powerLevel)));
        
        if (h != null) {
            for (Enumeration e1 = h.keys(); e1.hasMoreElements(); ) {
                Object elm = e1.nextElement();
                if (elm != null) {
                    retval.add(e1.nextElement());
                }
            }
        }
        
        return retval;
    }
    
    public Vector getSendMotes() {
        Vector retval = new Vector();
        
        for (Enumeration e1 = this.packetStats.keys(); e1.hasMoreElements(); ) {
            Hashtable recv = (Hashtable)(this.packetStats.get(e1.nextElement()));
            if (recv != null) {
                for (Enumeration e2 = recv.keys(); e2.hasMoreElements(); ) {
                    Hashtable send = (Hashtable)(recv.get(e2.nextElement()));
                    for (Enumeration e3 = send.keys(); e3.hasMoreElements(); ) {
                        Object elm = e3.nextElement();
                        if (elm != null && !retval.contains(elm)) {
                            retval.add(elm);
                        }
                    }
                }
            }
        }
        
        return retval;
    }
    
    public Vector getSendMotesAtPowerLevel(int powerLevel) {
        Vector retval = new Vector();

        Hashtable recv = (Hashtable)(this.packetStats.get(new Integer(powerLevel)));
        if (recv != null) {
            for (Enumeration e1 = recv.keys(); e1.hasMoreElements(); ) {
                Hashtable send = (Hashtable)(recv.get(e1.nextElement()));
                for (Enumeration e2 = send.keys(); e2.hasMoreElements(); ) {
                    Object elm = e2.nextElement();
                    if (elm != null && !retval.contains(elm)) {
                        retval.add(elm);
                    }
                }
            }
        }
        
        return retval;
    }
    
    public Vector getSendMotesAtPowerAndRecv(int powerLevel,Vector receivers) {
        Vector retval = new Vector();
        
        Hashtable recv = (Hashtable)(this.packetStats.get(new Integer(powerLevel)));
        if (recv != null) {
            for (Enumeration e1 = receivers.elements(); e1.hasMoreElements(); ) {
                Hashtable send = (Hashtable)(recv.get(e1.nextElement()));
                if (send != null) {
                    for (Enumeration e2 = send.keys(); e2.hasMoreElements(); ) {
                        Object elm = e2.nextElement();
                        if (elm != null && !retval.contains(elm)) {
                            retval.add(elm);
                        }
                    }
                }
            }
        }
        
        return retval;
    }
    
}
